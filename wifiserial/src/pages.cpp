//
// Created by marijn on 09/08/17.
//

#include <WiFi.h>
//#include "Arduino.h"
#include "pages.h"

void displayPage(WiFiClient client, ResponseCode_t response_code, const String &message, pageStyle_t style) {
    uint8_t response_msg_len = 23;
    char response_msg[response_msg_len];
    client.print("HTTP/1.1 ");
    client.print(response_code);
    Serial.print(response_code);
    client.print(" ");
    Serial.print(" ");
    switch (response_code) {
        case OkCode:
            strncpy(response_msg, "OK", response_msg_len);
            break;
        case NotFoundCode:
            strncpy(response_msg, "Not Found", response_msg_len);
            client.println();
            break;
        case UnsupportedMediaTypeCode:
            strncpy(response_msg, "Unsupported Media Type", response_msg_len);
            break;
        default:
            strncpy(response_msg, "Unknown Error", response_msg_len);
    }
    client.println(response_msg);
    client.println("Connection: close");
    client.println("Content-type: text/html; charset=utf-8");
    client.println();

    // the content of the HTTP response follows the header:
    client.println("<!DOCTYPE html><html><body>");
    switch(style) {
        case styleOkay:
            client.println("<div style=\"color: black\">");
            break;
        case styleError:
            client.println("<div style=\"color: red\">");
            break;
    }
    if(message.length() == 0) {
        client.print(response_msg);
        Serial.println(response_msg);
    } else {
        client.print(message);
        Serial.println(message);
    }
    client.print("</div></body></html>");

    // The HTTP response ends with another blank line:
    client.println();
}

void handleIndexPage(WiFiClient &client) {
    displayPage(client, OkCode, "Index page", styleOkay);
}

void handleHttpError(WiFiClient &client, ResponseCode_t error_code) {
    displayPage(client, error_code, "ERROR", styleError);
}