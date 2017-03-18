#include <stdio.h>
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

int main()
{
    char gif_filename[] = "test_3layers.gif";
    GifFileType *gif;
    int *gif_err = 0;
    gif = DGifOpenFileName(gif_filename, gif_err);
    if(gif == NULL) {
        printf("Can't read file '%s'. %d", gif_filename, *gif_err);
        return 1;
    }
    if(DGifSlurp(gif) == GIF_ERROR) {
        printf("problems parsing %s", gif_filename);
        return 1;
    }
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
