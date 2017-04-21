
#include <inttypes.h>
#include <WiFi.h>
#include "secret.h"
#include "giflib/gif_lib.h"

const char* ssid     = SSID_NAME;
const char* password = SSID_PASS;

#define DISPLAY_WIDTH 3*6*8
#define DISPLAY_ROWS 3

WiFiServer server(80);

void setup()
{
    Serial.begin(115200);
    pinMode(5, OUTPUT);      // set the LED pin mode

    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
    Serial.println("READY");
}

struct request_struct {
    String verb;
    String path;
    String content;
    String contentType;
    uint16_t contentLength;
};

void displayPage(WiFiClient client, String message) {
    client.println("HTTP/1.1 200 OK");
    client.println("Connection: close");
    client.println("Content-type: text/html; charset=utf-8");
    client.println();

    // the content of the HTTP response follows the header:
    client.print("<!DOCTYPE html><html><body>");
    client.print(message);
    client.print("</body></html>");

    // The HTTP response ends with another blank line:
    client.println();
}

void announceClient(WiFiClient client) {
    Serial.print("new client from ");        // print a message out the serial port
    Serial.print(client.remoteIP());
    Serial.print(":");
    Serial.println(client.remotePort());
}

void handleHeader(WiFiClient client, struct request_struct *request) {
    Serial.println("HEADER");
    String currentLine = "";           // make a String to hold incoming data from the client
    request->contentLength = 0;
    int lineNr = 0;
    while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
            char c = client.read();             // read a byte, then
            Serial.write(c);
            if(c == '\n') {
                if(currentLine.length() == 0) {
                    return;
                }
                int headerMark = currentLine.indexOf(':');
                if (headerMark != -1) {
                    String headerName  = currentLine.substring(0, headerMark);
                    String headerValue = currentLine.substring(headerMark+2);
                    if(headerName == "Content-Length") {
                        request->contentLength = headerValue.toInt();
                    }
                    if(headerName == "Content-Type") {
                        request->contentType = headerValue;
                    }
                } else if (lineNr == 0) {
                    uint16_t verb_space = currentLine.indexOf(' ');
                    request->verb = currentLine.substring(0, verb_space);
                    uint16_t path_space = currentLine.indexOf(' ', verb_space+1);
                    request->path  = currentLine.substring(verb_space+1, path_space);
                }
                lineNr++;
                currentLine = "";
            } else if (c != '\r') {  // if you got anything else but a carriage return character,
                currentLine += c;      // add it to the end of the currentLine
            }
        }
    }
}

void handleBody(WiFiClient client, struct request_struct *request) {
    Serial.println("BODY");
    uint16_t data_bytes = request->contentLength;
    while (client.connected() and --data_bytes > 0) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
            char c = client.read();             // read a byte, then
            request->content += c;
        }
    }
}

void printRequestResults(struct request_struct *request) {
    Serial.println("-------------");
    Serial.println(request->verb);
    Serial.println(request->path);
    Serial.println(request->content);
    Serial.print(request->contentLength);
    Serial.print(" bytes of ");
    Serial.println(request->contentType);
}

struct FakeHandle {
    String content;
    uint16_t pos;
};

static int content_read_func(GifFileType *ft, GifByteType *bt, int arg) {
    struct FakeHandle* data = (struct FakeHandle*) ft->UserData;
    // Serial.print("content_read_func reading from ");
    // Serial.print(data->pos);
    // Serial.print(" to ");
    // Serial.print(data->pos+arg);
    byte buf[arg];
    data->content.substring(data->pos, data->pos+arg).getBytes(buf, arg);
    Serial.print(". Sending: '");
    Serial.write(buf, arg);
    Serial.println("'.");

    memcpy(bt, buf, arg);
    data->pos += arg;
    return arg;
}

GifFileType* parseGif(String content) {
    GifFileType *gif;
    int *gif_err = 0;
    struct FakeHandle handle;
    handle.content = content;
    handle.pos = 0;
    //byte* data = (byte*)malloc(content.length());
    //content.getBytes(data, content.length());
    DGifOpen(&handle, &content_read_func, gif_err);
    if (gif == NULL || gif_err != 0)
    {
        Serial.print("Failure to parse GIF. Error: ");
        Serial.println(String(*gif_err));
        if (gif != NULL) {
            EGifCloseFile (gif, NULL);
        }
        return NULL; /* not a GIF */
    }
    // free(data);
    // data = NULL;
    if(gif == NULL) {
        Serial.println("Can't read file: %d", *gif_err);
        return NULL;
    }
    if(DGifSlurp(gif) == GIF_ERROR) {
        Serial.println("problems parsing gif");
        return NULL;
    }
    return gif;
}

int16_t get_frame_delay(SavedImage frame) {
    int16_t delay = -1;
    for(uint8_t t=0; t<frame.ExtensionBlockCount; t++) {
        ExtensionBlock block = frame.ExtensionBlocks[t];
        if(block.Function == 0xf9) {
            delay = (block.Bytes[2]*255 + block.Bytes[1])*10;
        }
    }
    return delay;
}

void handleClient(WiFiClient client) {
    struct request_struct request;
    handleHeader(client, &request);
    handleBody(client, &request);
    printRequestResults(&request);
    if(request.verb == "GET") {
        displayPage(client, "OK");
    } else if(request.verb == "POST" && request.contentType == "image/gif") {
        GifFileType *gif = parseGif(request.content);
        if(gif == NULL) {
            return;
        }
        GifWord width = gif->SWidth;
        GifWord height = gif->SHeight;
        if( (DISPLAY_WIDTH != width) || (DISPLAY_ROWS*8 != height) ) {
            Serial.println("Frame is wrong size");// %dx%d should be (%dx%d)\n",
                   //width, height, DISPLAY_WIDTH, DISPLAY_ROWS*8);
            return;
        }
        uint8_t display_bytes[DISPLAY_WIDTH*DISPLAY_ROWS];
        for(int frameNr=0; frameNr<gif->ImageCount; frameNr++) {
            SavedImage frame = gif->SavedImages[frameNr];
            int16_t frame_delay = get_frame_delay(frame);
            Serial.print("------ Frame ");
            Serial.print(frameNr);
            Serial.print(" shown for ");
            Serial.print(frame_delay);
            Serial.println("ms ------");
//            image_bytes(display_bytes, gif->SavedImages[frameNr].RasterBits);

        }
    }
}

void loop() {
    WiFiClient client = server.available();   // listen for incoming clients
    if (client) {
        announceClient(client);
        handleClient(client);
        client.stop();
        Serial.println("client disonnected");
    }
};

// ///////////////
// #include <stdio.h>
// #include <inttypes.h>
//
// FILE *fp;
//
// #define DISPLAY_WIDTH 3*6*8
// #define DISPLAY_ROWS 3
//
// void image_bytes(uint8_t *display_bytes, GifByteType *RasterBits) {
//     for(int j=0; j<(DISPLAY_WIDTH*DISPLAY_ROWS); j++) {
//         char b = 0;
//         unsigned char res = 0;
//         for(int i=0; i<8; i++) {
// //            int pos = j+(i*width);
//             b = RasterBits[j+(i*DISPLAY_WIDTH)];
//             res = res | (b << i);
// //            printf("pos=%d; b=%d; res=%x\n", pos, b, res);
//         }
//         display_bytes[j] = res;
//         printf("%02x", display_bytes[j]);
//     }
//     printf("\n");
// }
//
// void print_bytes(uint8_t *display_bytes) {
//     for(int k=0; k<(DISPLAY_WIDTH*DISPLAY_ROWS); k++) {
//         printf("%02x", display_bytes[k]);
//     }
//     printf("\n");
// }
//
// int16_t get_frame_delay(SavedImage frame) {
//     int16_t delay = -1;
//     for(uint8_t t=0; t<frame.ExtensionBlockCount; t++) {
//         ExtensionBlock block = frame.ExtensionBlocks[t];
//         if(block.Function == 0xf9) {
//             delay = (block.Bytes[2]*255 + block.Bytes[1])*10;
//         }
//     }
//     return delay;
// }
//
// int main()
// {
//     char gif_filename[] = "test_3layers.gif";
//     GifFileType *gif;
//     int *gif_err = 0;
//     gif = DGifOpenFileName(gif_filename, gif_err);
//     if(gif == NULL) {
//         printf("Can't read file '%s'. %d", gif_filename, *gif_err);
//         return 1;
//     }
//     if(DGifSlurp(gif) == GIF_ERROR) {
//         printf("problems parsing %s", gif_filename);
//         return 1;
//     }
//     GifWord width = gif->SWidth;
//     GifWord height = gif->SHeight;
//     if( (DISPLAY_WIDTH != width) || (DISPLAY_ROWS*8 != height) ) {
//         printf("Frame is wrong size %dx%d should be (%dx%d)\n",
//                width, height, DISPLAY_WIDTH, DISPLAY_ROWS*8);
//         return 1;
//     }
//     uint8_t display_bytes[DISPLAY_WIDTH*DISPLAY_ROWS];
//     for(int frameNr=0; frameNr<gif->ImageCount; frameNr++) {
//         SavedImage frame = gif->SavedImages[frameNr];
//         int16_t frame_delay = get_frame_delay(frame);
//         printf("------ Frame %d shown for %dms ------\n", frameNr, frame_delay);
//         image_bytes(display_bytes, gif->SavedImages[frameNr].RasterBits);
//         //print_bytes(display_bytes);
//     }
//     return 0;
// }
