CFLAGS = -c -Wall -g

LD_VERSION = $(shell ld -v 2>&1 | sed -ne 's/.*\([0-9]\+\.[0-9]\+\).*/\1/p')
ifeq "$(LD_VERSION)" "2.34"
	CFLAGS += -D HAS_BINUTILS_234
endif

all: test.o crash.o cLog.o
	g++ test.o crash.o cLog.o -rdynamic -ldl -lbfd  -o test

cLog.o: ../utils/cLog.cpp
	g++ -std=c++17 $(CFLAGS) ../utils/cLog.cpp -o cLog.o

test.o: test.cpp crash.h
	g++ -std=c++17 $(CFLAGS) test.cpp -o test.o

crash.o: crash.cpp crash.h
	g++ -std=c++17 $(CFLAGS) crash.cpp -o crash.o

clean:
	rm -rf *.o test

rebuild:
	make clean && make
