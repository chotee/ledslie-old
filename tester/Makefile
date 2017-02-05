C=gcc
CFLAGS=-I.
DEPS =

%.o: %.c $(DEPS)
	$(CC) -ggdb -c -o $@ $< $(CFLAGS)

test_refresher: test_refresher.o
	gcc -o test_refresher test_refresher.o -I.

all: test_refresher
