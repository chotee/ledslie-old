#include "Arduino.h"
#include <WiFi.h>
#include "secret.h"
#include "../lib/giflib/gif_lib.h"

const char* ssid     = SSID_NAME;
const char* password = SSID_PASS;

#define DISPLAY_WIDTH (3*6*8)
#define DISPLAY_ROWS 3
#define SERIAL_DEBUG_BAUD 115200

#define LED1_SERIAL_BAUD 115200
#define LED1_SERIAL_TX 17
#define LED1_SERIAL_RX 16

WiFiServer server(80);

HardwareSerial Serial1(1);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
void setup()
{
    Serial.begin(SERIAL_DEBUG_BAUD);
    Serial1.begin(LED1_SERIAL_BAUD, SERIAL_8N1, 16, 17);
    pinMode(5, OUTPUT);      // set the LED pin mode

    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    // WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint16_t nr = 1;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if(nr++ % 10 == 0) {
            WiFi.reconnect();
            Serial.println(" restarting");
            delay(500);
        }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
    Serial.println("READY");
}
#pragma clang diagnostic pop

struct request_struct {
    String verb;
    String path;
    bool expect_header = false;
    String content;
    String contentType;
    long contentLength = 0;
    long pos = 0;
};

void displayPage(WiFiClient client, String message) {
    client.println("HTTP/1.1 200 OK");
    client.println("Connection: close");
    client.println("Content-type: text/html; charset=utf-8");
    client.println();

    // the content of the HTTP response follows the header:
    client.print("<!DOCTYPE html><html><body>");
    client.print(message);
    Serial.println(message);
    client.print("</body></html>");

    // The HTTP response ends with another blank line:
    client.println();
}

void announceClient(WiFiClient client) {
    Serial.print("New client from ");        // print a message out the serial port
    Serial.print(client.remoteIP());
    Serial.print(":");
    Serial.println(client.remotePort());
}

void handleHeader(WiFiClient client, struct request_struct *request) {
    Serial.println("HEADER");
    String currentLine = "";           // make a String to hold incoming data from the client
    request->contentLength = 0;
    request->expect_header = false;
    int lineNr = 0;
    // Serial.print("handleHeader Connected: ");
    // Serial.println(client.connected());
    while ((boolean)client.connected()) {            // loop while the client's connected
        if (client.available() > 0) {             // if there's bytes to read from the client,
            char c = (char) client.read();             // read a byte, then
            Serial.write(c);
            if(c == '\n') {
                if(currentLine.length() == 0) {
                    return;
                }
                unsigned int headerMark = currentLine.indexOf(':');
                if (headerMark != -1) {
                    String headerName  = currentLine.substring(0, headerMark);
                    String headerValue = currentLine.substring(headerMark+2);
                    if(headerName == "Content-Length") {
                        request->contentLength = headerValue.toInt();
                    }
                    else if(headerName == "Content-Type") {
                        request->contentType = headerValue;
                    }
                    else if(headerName == "Expect") {
                        request->expect_header = true;
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
    // Serial.println("BODY");
    uint16_t nr = 1;
    uint16_t data_bytes = request->contentLength;
    // Serial.print("contentLength data_bytes=");
    // Serial.println(data_bytes);
    while (client.connected() && data_bytes-- > 0) {            // loop while the client's connected
        // Serial.print("c");
        if (client.available()) {             // if there's bytes to read from the client,
            // Serial.print("a");
            char c = client.read();             // read a byte, then
            request->content += c;
            nr++;
        }
    }
    Serial.print("Stored bytes: ");
    Serial.println(request->content.length());
}

void handleContinue(WiFiClient client, struct request_struct *request) {
    if(request->expect_header == true) {
        client.println("HTTP/1.1 100 Continue");
        client.println();
        Serial.println("Send Continue response.");
    }
}

void printRequestResults(struct request_struct *request) {
    Serial.println("-------------");
    Serial.println(request->verb);
    Serial.println(request->path);
    // Serial.print("Content '");
    // Serial.print(request->content);
    // Serial.println("'.");
    Serial.print(request->contentLength);
    Serial.print(" bytes of ");
    Serial.println(request->contentType);
    for(uint16_t i=0; i<request->content.length(); i++) {
        if(i % 16 == 0) {
            Serial.printf("\n%04x |", i);
        }
        Serial.printf("%02x", int(request->content.charAt(i)));
        Serial.print(" ");
        if(i > 0){
            if(i % 8 == 0 && i % 16 != 0) {
                Serial.print(" || ");
            }
        }
    }
    Serial.println();
    Serial.println("----");
}

static int content_read_func(GifFileType *ft, GifByteType *bt, int arg) {
    struct request_struct* request = (struct request_struct*) ft->UserData;
    const char* buf = request->content.c_str() + request->pos;
    memcpy(bt, buf, arg);
    request->pos += arg;
    return arg;
}

GifFileType* parseGifHeader(struct request_struct *request) {
    Serial.println("parseGif");
    GifFileType *gif;
    int gif_err = 0;
    request->pos = 0;
    gif = DGifOpen(request, &content_read_func, &gif_err);
    if (gif == NULL || gif_err != 0)
    {
        Serial.print("Failure to parse GIF. Error");
        Serial.println(GifErrorString(gif_err));

        return NULL; /* not a GIF */
    }
    if(gif == NULL) {
        Serial.println("Can't read file.");
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

void image_bytes(uint8_t *display_bytes, GifByteType *RasterBits) {
    for(int j=0; j<(DISPLAY_WIDTH*DISPLAY_ROWS); j++) {
        char b = 0;
        unsigned char res = 0;
        for(int i=0; i<8; i++) {
//            int pos = j+(i*width);
            b = RasterBits[j+(i*DISPLAY_WIDTH)];
            res = res | (b << i);
//            printf("pos=%d; b=%d; res=%x\n", pos, b, res);
        }
        display_bytes[j] = res;
        printf("%02x", display_bytes[j]);
    }
    printf("\n");
}

void handleRequest(WiFiClient client, struct request_struct* request) {
    handleHeader(client, request); // Pull information from the HTTP header, store in the request object.
    handleContinue(client, request); // If it expects it, send a 100-Continue message to the sender.
    handleBody(client, request); // Handle the body, loading it into the request object.
    printRequestResults(request); // Print information on the request.
}

GifFileType * formulateResponse(WiFiClient client, struct request_struct* request) {
    const uint8_t err_msg_length = 80;
    char msg[err_msg_length]; // Location to put the message.
    int gif_process_res = 0;
    if(request->verb == "GET") {
        displayPage(client, "OK");
    } else if(request->verb == "POST" && request->contentType == "image/gif") {
        // Serial.println("it was a POST");
        GifFileType *gif = parseGifHeader(request); // process the header of the gif.
        if(gif == NULL) {
            return NULL; // Something went wrong processing the header. Not a GIF?
        }
        GifWord width = gif->SWidth;
        GifWord height = gif->SHeight;
        GifWord colors = gif->SColorResolution;
        // Determine if the GIF is correct for our application.
        if( (DISPLAY_WIDTH != width) || (DISPLAY_ROWS*8 != height) ) {
            // Image as the wrong size
            snprintf(msg, err_msg_length, "Frame is wrong size %dx%d. it should be (%dx%d)", width, height, DISPLAY_WIDTH, DISPLAY_ROWS*8);
            displayPage(client, msg);
            return NULL;
        } else if(colors != 1) {
            // Image is not mono-chrome.
            snprintf(msg, err_msg_length, "Number of colors %d is unsupported. Should be 1", colors);
            displayPage(client, msg);
            return NULL;
        } else {
            // Image header seems to match expectations. Return info for debugging.
            snprintf(msg, err_msg_length, "Color depth: %d. Size is %dx%d.", colors, width, height);
            displayPage(client, msg);
        }
        gif_process_res = DGifSlurp(gif); // Process the image's data.
        if(gif_process_res == GIF_ERROR) {
            snprintf(msg, err_msg_length, "Problems parsing gif. %s", GifErrorString(gif->Error));
            displayPage(client, msg);
            return NULL;
        }
        return gif;
    }
    snprintf(msg, err_msg_length, "%s request was not correct.", request->verb);
    displayPage(client, msg);
    return NULL;
}

void sendImage(GifFileType *gif) {
    uint8_t display_bytes[DISPLAY_WIDTH*DISPLAY_ROWS];
    for(int frameNr=0; frameNr<gif->ImageCount; frameNr++) {
        SavedImage frame = gif->SavedImages[frameNr];
        int16_t frame_delay = get_frame_delay(frame);
        Serial.print("------ Frame ");
        Serial.print(frameNr);
        Serial.print(" shown for ");
        Serial.print(frame_delay);
        Serial.println("ms ------");
        image_bytes(display_bytes, gif->SavedImages[frameNr].RasterBits);
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
void loop() {
    WiFiClient client = server.available();   // listen for incoming clients
    GifFileType *gif = NULL;
    int gif_err = 0;
    char msg[80];
    struct request_struct request;
    if (client) {
        request.content.remove(0); // Resetting it to empty.
        request.pos = 0;
        request.contentLength = 0;
        if(gif != NULL) { // start clean.
            gif = NULL;
        }
        announceClient(client); // just for debugging. Report on the client connecting
        handleRequest(client, &request); // Get all of the request information out of the request
        gif = formulateResponse(client, &request); // process the send data, returning the gif object processed.
        client.stop(); // Close the connection
        Serial.println("client disonnected");
    }
    if(gif != NULL) {
        sendImage(gif);
        if(DGifCloseFile(gif, &gif_err) == GIF_ERROR) {

            snprintf(msg, 80, "Problems freeing gif. %s", GifErrorString(gif_err));
            displayPage(client, msg);
        }
    }
    gif = NULL;
};
#pragma clang diagnostic pop
