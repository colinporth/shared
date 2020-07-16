CFLAGS = -c -Wall

all: test.o crash.o
	g++ test.o crash.o -rdynamic -ldl -o test

test.o:
	g++ -std=c++17 $(CFLAGS) test.cpp -o test.o

crash.o:
	g++ -std=c++17 $(CFLAGS) crash.cpp -o crash.o

clean:
	rm -rf *.o test
