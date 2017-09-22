#include "Arduino.h"
#include <WiFi.h>
#include "../lib/giflib/gif_lib.h"
#include "secret.h"
#include "pages.h"

const char* ssid     = SSID_NAME;
const char* password = SSID_PASS;

#define DISPLAY_WIDTH (3*6*8)
#define DISPLAY_ROWS 3
#define SERIAL_DEBUG_BAUD 115200

#define LED1_SERIAL_BAUD 115200
//#define LED1_SERIAL_TX 17
//#define LED1_SERIAL_RX 16

#define ERROR_MSG_LENGTH 80

WiFiServer server(80);
HardwareSerial Serial1(1);

constexpr unsigned int str2int(const char *str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

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
    String verb; // Verb of the request, POST, GET, etc.
    String path; // Path of the request
    bool expect_header = false; // True if the caller expects a 100 continue.
    String contentType; // mime-type of the content.
    uint16_t contentLength = 0; // Number of bytes of the content as reported by the client.

    // Used for the loading of the gif
    WiFiClient *client = nullptr; // The client object.
    uint16_t pos = 0; // Location where the client is reading the file
    char err_msg[ERROR_MSG_LENGTH] = "";
};

struct image_queue_struct {
    uint32_t curr_frame_start = 0;
    GifFileType *gif = nullptr;

};

void parse_start_line(request_struct *request, const String &currentLine);

void parse_header_line(request_struct *request, const String &currentLine);

Request_t decideRequestType(request_struct *request);

char * verify_image_header(struct request_struct *request, GifFileType *gif) ;

void announceClient(WiFiClient &client) {
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
        if (client.available() > 0) {                // if there's bytes to read from the client,
            char c = (char) client.read();           // read a byte, then
            Serial.write(c);
            if(c == '\n') { // This line is now done. See what it contains.
                if(currentLine.length() == 0) {
                    return; // Double newline. End of the headers section of the request.
                }
                if (lineNr == 0) { // This is the first line, not a header. Treat it differently.
                    parse_start_line(request, currentLine);
                } else {
                    parse_header_line(request, currentLine);
                }
                lineNr++;
                currentLine = "";
            } else if (c != '\r') {  // if you got anything else but a carriage return character,
                currentLine += c;      // add it to the end of the currentLine
            }
        }
    }
}

void parse_header_line(request_struct *request, const String &currentLine) {
    unsigned int headerMark = currentLine.indexOf(':');
    if (headerMark != -1) { // If we can't find the color, we're skipping this line.
        String headerName = currentLine.substring(0, headerMark);
        String headerValue = currentLine.substring(headerMark + 2);
        if (headerName == "Content-Length") {
            request->contentLength = headerValue.toInt();
        } else if (headerName == "Content-Type") {
            request->contentType = headerValue;
        } else if (headerName == "Expect") {
            request->expect_header = true;
        }
    }
}

void parse_start_line(request_struct *request, const String &currentLine) {
    int verb_space = currentLine.indexOf(' ');
    request->verb = currentLine.substring(0, verb_space);
    int path_space = currentLine.indexOf(' ', verb_space+1);
    request->path  = currentLine.substring(verb_space+1, path_space);
}

static int content_read_func(GifFileType *ft, // Structure.
                             GifByteType *bt, // Buffer to write bytes to.
                             int arg // Expected number of bytes to return.
) {
    struct request_struct* request = (struct request_struct*) ft->UserData;
    WiFiClient *client = request->client;
    int i = 0;
    while (client->connected() != 0u // loop while the client's connected
           && arg > i ) {           // and number of bytes requested in the call is not yet complete
        if (client->available() != 0) {                   // if there's bytes to read from the client,
            *(bt+i) = (GifByteType) client->read();       // read a byte, store in the bt buffer.
            i++;
        }
    }
    request->pos += i;
    return i;
}

GifFileType * handleImageUpload(WiFiClient client, struct request_struct *request) {
    GifFileType *gif = nullptr;
    int gif_err = 0;
    request->client = &client;
    request->pos = 0;
    gif = DGifOpen(request, &content_read_func, &gif_err);
    if(gif == nullptr) {
        strncpy(request->err_msg, "Not a valid gif.", ERROR_MSG_LENGTH);
        return nullptr; // Something went wrong processing the header. Not a GIF?
    }
    char * err_msg = verify_image_header(request, gif);
    if(err_msg) {
        return nullptr;
    }
    int gif_process_res = DGifSlurp(gif); // Process the image's data.
    if(gif_process_res == GIF_ERROR) {
        snprintf(request->err_msg, ERROR_MSG_LENGTH, "Problems parsing gif. %s", GifErrorString(gif->Error));
        return nullptr;
    }
    return gif;

}

char * verify_image_header(struct request_struct *request, GifFileType *gif) {
    strncpy(request->err_msg, "", ERROR_MSG_LENGTH); // Cleaning out the error message.
    // Determine if the GIF is correct for our application.
    if( (DISPLAY_WIDTH != gif->SWidth) || (DISPLAY_ROWS*8 != gif->SHeight) ) {
        // Image as the wrong size
        snprintf(request->err_msg, ERROR_MSG_LENGTH, "Frame is wrong size %dx%d. it should be (%dx%d)",
                 gif->SWidth, gif->SHeight, DISPLAY_WIDTH, DISPLAY_ROWS * 8);
    }
    else if(gif->SColorResolution != 1) {
        // Image is not mono-chrome.
        snprintf(request->err_msg, ERROR_MSG_LENGTH,
                 "Number of colors %d is unsupported. Should be 1", gif->SColorResolution);
    }
}

void handleContinue(WiFiClient client, struct request_struct *request) {
    if(request->expect_header) {
        client.println("HTTP/1.1 100 Continue");
        client.println();
        Serial.println("Send Continue response.");
    }
}

void printRequestResults(struct request_struct *request) {
    Serial.println("-------------");
    Serial.println(request->verb);
    Serial.print(" ");
    Serial.println(request->path);
    // Serial.print("Content '");
    // Serial.print(request->content);
    // Serial.println("'.");
    Serial.print(request->contentLength);
    Serial.print(" bytes of ");
    Serial.println(request->contentType);
//    for(uint16_t i=0; i<request->content.length(); i++) {
//        if(i % 16 == 0) {
//            Serial.printf("\n%04x |", i);
//        }
//        Serial.printf("%02x", int(request->content.charAt(i)));
//        Serial.print(" ");
//        if(i > 0){
//            if(i % 8 == 0 && i % 16 != 0) {
//                Serial.print(" || ");
//            }
//        }
//    }
//    Serial.println();
    Serial.println("----");
}
//
//GifFileType* parseGifHeader(struct request_struct *request) {
//    Serial.println("parseGif");
//    GifFileType *gif;
//    int gif_err = 0;
//    request->pos = 0;
//    gif = DGifOpen(request, &content_read_func, &gif_err);
//    if (gif == NULL || gif_err != 0)
//    {
//        Serial.print("Failure to parse GIF. Error");
//        Serial.println(GifErrorString(gif_err));
//
//        return NULL; /* not a GIF */
//    }
//    if(gif == NULL) {
//        Serial.println("Can't read file.");
//        return NULL;
//    }
//    return gif;
//}

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

GifFileType * handleRequest(WiFiClient client, struct request_struct* request) {
    GifFileType * gif = nullptr;
    handleHeader(client, request); // Pull information from the HTTP header, store in the request object.
    handleContinue(client, request); // If it expects it, send a 100-Continue message to the sender.
    printRequestResults(request); // Print information on the request.
    switch (decideRequestType(request)) {
        case ImageUploadRequest:
            gif = handleImageUpload(client, request); // Handle the body, loading it into the request object.
            if(gif == nullptr) {
                displayPage(client, NotAcceptableCode, request->err_msg, styleError);
            }
            break;
        case IndexPageRequest:
            handleIndexPage(client);
            break;
        case UnsupportedMediaTypeRequest:
            handleHttpError(client, UnsupportedMediaTypeCode);
            break;
        case UnknownPageTypeRequest:
        default:
            handleHttpError(client, NotFoundCode);
    }
    return gif;
}

Request_t decidePOSTRequestType(request_struct *request) {
    uint8_t buflen = 10;
    char contentType[buflen];
    request->contentType.toCharArray(contentType, buflen);
    switch(str2int(contentType)) {
        case str2int("image/gif"):
            return ImageUploadRequest;
    }
    return  UnsupportedMediaTypeRequest;
}

Request_t decideGETRequestType(request_struct *request) {
    uint8_t buflen = 10;
    char path[buflen];
    request->path.toCharArray(path, buflen);
    switch (str2int(path)) {
        case str2int("/"):
            return IndexPageRequest;
    }
    return UnknownPageTypeRequest;
}

Request_t decideRequestType(request_struct *request) {
    uint8_t buflen = 10;
    char verb[buflen];
    request->verb.toCharArray(verb, buflen);
    switch(str2int(verb)) {
        case str2int("POST"):
            return decidePOSTRequestType(request);
        case str2int("GET"):
            return decideGETRequestType(request);
    }
    return UnknownPageTypeRequest;
}

//GifFileType * formulateResponse(WiFiClient client, struct request_struct* request) {
//    const uint8_t err_msg_length = 80;
//    char msg[err_msg_length]; // Location to put the message.
//    int gif_process_res = 0;
//    if(request->verb == "GET") {
////        displayPage(client, OkCode, "OK", styleOkay);
//    } else if(request->verb == "POST" && request->contentType == "image/gif") {
//        // Serial.println("it was a POST");
//        GifFileType *gif = parseGifHeader(request); // process the header of the gif.
//        if(gif == nullptr) {
//            return nullptr; // Something went wrong processing the header. Not a GIF?
//        }
//        GifWord width = gif->SWidth;
//        GifWord height = gif->SHeight;
//        GifWord colors = gif->SColorResolution;
//        // Determine if the GIF is correct for our application.
//        if( (DISPLAY_WIDTH != width) || (DISPLAY_ROWS*8 != height) ) {
//            // Image as the wrong size
//            snprintf(msg, err_msg_length, "Frame is wrong size %dx%d. it should be (%dx%d)", width, height, DISPLAY_WIDTH, DISPLAY_ROWS*8);
////            displayPage(client, msg);
//            return nullptr;
//        } else if(colors != 1) {
//            // Image is not mono-chrome.
//            snprintf(msg, err_msg_length, "Number of colors %d is unsupported. Should be 1", colors);
////            displayPage(client, msg);
//            return nullptr;
//        } else {
//            // Image header seems to match expectations. Return info for debugging.
//            snprintf(msg, err_msg_length, "Color depth: %d. Size is %dx%d.", colors, width, height);
////            displayPage(client, msg);
//        }
//        gif_process_res = DGifSlurp(gif); // Process the image's data.
//        if(gif_process_res == GIF_ERROR) {
//            snprintf(msg, err_msg_length, "Problems parsing gif. %s", GifErrorString(gif->Error));
////            displayPage(client, msg);
//            return nullptr;
//        }
//        return gif;
//    }
//    snprintf(msg, err_msg_length, "%s request was not correct.", request->verb);
////    displayPage(client, msg);
//    return nullptr;
//}

void queueImage(GifFileType *gif) {
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
    GifFileType *gif = nullptr;
    int gif_err = 0;
    char msg[80];
    struct request_struct request;
    if (client) {
        request.pos = 0;
        request.contentLength = 0;
        announceClient(client); // just for debugging. Report on the client connecting
        gif = handleRequest(client, &request); // Get all of the request information out of the request
        client.stop(); // Close the connection
        Serial.println("client disonnected");
    }
    if(gif != nullptr) {
        queueImage(gif);
        if(DGifCloseFile(gif, &gif_err) == GIF_ERROR) {
            snprintf(msg, 80, "Problems freeing gif. %s", GifErrorString(gif_err));
        }
    }
    gif = NULL;
};

#pragma clang diagnostic pop
