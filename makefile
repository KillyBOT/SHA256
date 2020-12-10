cc = gcc

all: SHA256.o
	$(CC) -o SHA256 SHA256.o -lm

SHA256.o: SHA256.c
	$(CC) -c SHA256.c -lm

clean:
	rm *.o