CFLAGS = -c -Wall -g

all: test.o crash.o
	g++ test.o crash.o -rdynamic -ldl -lbfd -o test

test.o: test.cpp
	g++ -std=c++17 $(CFLAGS) test.cpp -o test.o

crash.o: crash.cpp
	g++ -std=c++17 $(CFLAGS) crash.cpp -o crash.o

clean:
	rm -rf *.o test
