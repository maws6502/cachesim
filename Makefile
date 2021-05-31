CC=clang
CFLAGS=-I. -Wall -Wno-parentheses --std=c89 # fight me
DEPS = csim.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: csim csi

csim: main.o csim.o
	$(CC) -o $@ $^ $(CFLAGS)

csi: csi.o csim.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o csim csi