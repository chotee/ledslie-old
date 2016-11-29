C=gcc
CFLAGS=-I.
DEPS =

%.o: %.c $(DEPS)
	$(CC) -ggdb -c -o $@ $< $(CFLAGS)

tester: tester.o
	gcc -o tester tester.o -I.
