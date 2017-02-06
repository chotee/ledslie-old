C=gcc
CFLAGS=-I. -Wall -g -std=gnu99
DEPS =

%.o: %.c $(DEPS)
	$(CC) -ggdb -c -o $@ $< $(CFLAGS)

SRCS = giflib/dgif_lib.c giflib/egif_lib.c giflib/gifalloc.c giflib/gif_err.c giflib/gif_font.c giflib/gif_hash.c giflib/openbsd-reallocarray.c giflib/quantize.c

OBJS = $(SRCS:.c=.o)

all: test_refresher test_gifreader

giflib: $(OBJS)


test_refresher: test_refresher.o
	gcc -o test_refresher test_refresher.o -I.

test_gifreader: giflib test_gifreader.o
	gcc -o test_gifreader test_gifreader.o $(OBJS) -I.


clean:
	- rm test_refresher test_gifreader *.o
