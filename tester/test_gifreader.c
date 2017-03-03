#include <stdio.h>
#include <inttypes.h>
#include "giflib/gif_lib.h"

FILE *fp;

#define DISPLAY_WIDTH 3*6*8
#define DISPLAY_ROWS 3

int main()
{
    char gif_filename[] = "test_2layers.gif";
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

    GifWord width = gif->SavedImages->ImageDesc.Width;
    GifWord height = gif->SavedImages->ImageDesc.Height;
    uint8_t display_bytes[DISPLAY_WIDTH*DISPLAY_ROWS];
    for(int j=0; j<(DISPLAY_WIDTH*DISPLAY_ROWS); j++) {
        char b = 0;
        unsigned char res = 0;
        for(int i=0; i<8; i++) {
//            int pos = j+(i*width);
            b = gif->SavedImages->RasterBits[j+(i*DISPLAY_WIDTH)];
            res = res | (b << i);
//            printf("pos=%d; b=%d; res=%x\n", pos, b, res);
        }
        display_bytes[j] = res;
        printf("%02x\n", display_bytes[j]);
    }
    printf("\n");
    return 0;
}
