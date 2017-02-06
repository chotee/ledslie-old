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
    for(int i=0; i<8*8; i++) {
        if(i > 0 && i % 8 == 0) {
            printf("\n");
        }
        int b = gif->SavedImages->RasterBits[i];
        printf("%d", b);
    }
    printf("\n");
    return 0;
}
