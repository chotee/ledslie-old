#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "giflib/gif_lib.h"

FILE *fp;

#define DISPLAY_WIDTH 3*6*8
#define DISPLAY_ROWS 3

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

void print_bytes(uint8_t *display_bytes) {
    for(int k=0; k<(DISPLAY_WIDTH*DISPLAY_ROWS); k++) {
        printf("%02x", display_bytes[k]);
    }
    printf("\n");
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

struct FakeHandle {
    char* content;
    uint16_t pos;
};

static int content_read_func(GifFileType *ft, GifByteType *bt, int arg) {
    struct FakeHandle* data = (struct FakeHandle*) ft->UserData;
    printf("content_read_func reading from %d to %d\n", data->pos, data->pos+arg);
    //data->content.substring(data->pos, data->pos+arg).getBytes(buf, arg);

    // Serial.print(". Sending: '");
    // Serial.write(buf, arg);
    // Serial.println("'.");
    memcpy(bt, data->content+data->pos, arg);
    data->pos += arg;
    return arg;
}

GifFileType* parseGif(char * content) {
    GifFileType *gif;
    int *gif_err = 0;
    struct FakeHandle handle;
    handle.content = content;
    handle.pos = 0;
    //byte* data = (byte*)malloc(content.length());
    //content.getBytes(data, content.length());
    gif = DGifOpen(&handle, &content_read_func, gif_err);
    if (gif == NULL || gif_err != 0) {
       printf("Failure to parse GIF. Error: ");
    //    printf(*gif_err);
        if (gif != NULL) {
            EGifCloseFile (gif, NULL);
        }
        return NULL; /* not a GIF */
    }
    // free(data);
    // data = NULL;
    if(gif == NULL) {
        printf("Can't read file: %d", *gif_err);
        return NULL;
    }
    if(DGifSlurp(gif) == GIF_ERROR) {
        printf("problems parsing gif");
        return NULL;
    }
    return gif;
}

char* read_file(char* file_name) {
    FILE *ifp;
    ifp = fopen(file_name, "rb");
    fseek(ifp, 0, SEEK_END);
    long length = ftell(ifp);
    fseek(ifp, 0, SEEK_SET);
    char *bytes = (char*) malloc(length);
    fread(bytes, length, 1, ifp);
    fclose(ifp);
    return bytes;
}

int main()
{
    char gif_filename[] = "test_2layers.gif";
    GifFileType *gif;
    gif = parseGif(read_file(gif_filename));
    if(gif == NULL) {
        printf("FAILED.");
        return 1;
    }
    //gif = DGifOpenFileName(gif_filename, gif_err);
    GifWord width = gif->SWidth;
    GifWord height = gif->SHeight;
    if( (DISPLAY_WIDTH != width) || (DISPLAY_ROWS*8 != height) ) {
        printf("Frame is wrong size %dx%d should be (%dx%d)\n",
               width, height, DISPLAY_WIDTH, DISPLAY_ROWS*8);
        return 1;
    }
    uint8_t display_bytes[DISPLAY_WIDTH*DISPLAY_ROWS];
    for(int frameNr=0; frameNr<gif->ImageCount; frameNr++) {
        SavedImage frame = gif->SavedImages[frameNr];
        int16_t frame_delay = get_frame_delay(frame);
        printf("------ Frame %d shown for %dms ------\n", frameNr, frame_delay);
        image_bytes(display_bytes, gif->SavedImages[frameNr].RasterBits);
        //print_bytes(display_bytes);
    }
    return 0;
}
