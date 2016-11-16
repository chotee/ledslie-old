C=gcc
CFLAGS=-I.
DEPS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tester: tester.o
	gcc -o tester tester.o -I.
