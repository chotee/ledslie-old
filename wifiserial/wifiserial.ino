
#include <WiFi.h>
#include "secret.h"
#include <inttypes.h>

const char* ssid     = SSID_NAME;
const char* password = SSID_PASS;

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
    //Serial.println(request.content);
    Serial.print(request->contentLength);
    Serial.print(" bytes of ");
    Serial.println(request->contentType);
}

void handleClient(WiFiClient client) {
    struct request_struct request;
    handleHeader(client, &request);
    handleBody(client, &request);
    printRequestResults(&request);
    if(request.verb == "GET") {
        displayPage(client, "OK");
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
// if (client) {                             // if you get a client,
//     while (client.connected()) {            // loop while the client's connected
//       if (client.available()) {             // if there's bytes to read from the client,
//         char c = client.read();             // read a byte, then
//         Serial.write(c);                    // print it out the serial monitor
//         if (c == '\n') { // if the byte is a newline character
//           // if the current line is blank, you got two newline characters in a row.
//           // that's the end of the client HTTP request, so send a response:
//           if (currentLine.length() == 0) {
//             clientState = BODY;
//             // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
//             // and a content-type so the client knows what's coming, then a blank line:
//             // break out of the while loop:
//             break;
//           } else {    // if you got a newline, then clear currentLine:
//               int headerMark = currentLine.indexOf(':');
//               if (headerMark != -1) {
//                   String headerName  = currentLine.substring(0, headerMark);
//                   String headerValue = currentLine.substring(headerMark+2);
//                   Serial.print("\"");
//                   Serial.print(headerName);
//                   Serial.println("\"");
//                   Serial.print("\" = \"");
//                   Serial.print(headerValue);
//                   Serial.println("\"");
//                   if(headerName == "Content-Length") {
//                       contentLength = headerValue.toInt();
//                   }
//               }
//             currentLine = "";
//           }
//         } else if (c != '\r') {  // if you got anything else but a carriage return character,
//           currentLine += c;      // add it to the end of the currentLine
//         }
//         // // Check to see if the client request was "GET /H" or "GET /L":
//         // if (currentLine.endsWith("GET /H")) {
//         //   digitalWrite(5, HIGH);               // GET /H turns the LED on
//         // }
//         // if (currentLine.endsWith("GET /L")) {
//         //   digitalWrite(5, LOW);                // GET /L turns the LED off
//         // }
//       }
//     }
//     // close the connection:
//     client.stop();
//     Serial.println("client disonnected");
//   }
// }
