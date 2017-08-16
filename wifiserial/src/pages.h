//
// Created by marijn on 09/08/17.
//

#ifndef LEDSLIE_PAGES_H
#define LEDSLIE_PAGES_H

enum Request_t : int {
    IndexPageRequest,
    ImageUploadRequest,
    UnsupportedMediaTypeRequest,
    UnknownPageTypeRequest,
};
enum pageStyle_t : int {
    styleOkay,
    styleError,
};
enum ResponseCode_t : uint16_t {
    OkCode = 200,
    NotFoundCode = 404,
    NotAcceptableCode = 406,
    UnsupportedMediaTypeCode = 415,

};

void displayPage(WiFiClient client, ResponseCode_t response_code, const String &message, pageStyle_t style);

void handleIndexPage(WiFiClient &client);

void handleHttpError(WiFiClient &client, ResponseCode_t error_code);

#endif //LEDSLIE_PAGES_H
