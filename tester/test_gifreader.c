#include <stdio.h>
#include "giflib/gif_lib.h"

FILE *fp;

int main()
{
    char gif_filename[] = "test.gif";
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
    char b = 0;
    for(int j=0; j<8; j++) {
        unsigned char res = 0;
        for(int i=0; i<8; i++) {
            int pos = j+(i*width);
            b = gif->SavedImages->RasterBits[j+(i*width)];
            res = res | (b << i);
            printf("pos=%d; b=%d; res=%x\n", pos, b, res);
        }
        printf("%02x\n", res, res);
    }
    printf("\n");
    return 0;
}
