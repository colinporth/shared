CC = g++

CFLAGS = -c -Wall -I.

all: callStack.o test.o
	$(CC) callStack.o test.o -ldl -rdynamic -o test

callStack.o:
	$(CC) $(CFLAGS) callStack.cpp -o callStack.o

test.o:
	$(CC) $(CFLAGS) test.cpp -o test.o

clean:
	rm -rf *.o test
