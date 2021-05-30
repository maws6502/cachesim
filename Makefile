CC=clang
CFLAGS=-I. -Wall -Wno-parentheses --std=c89 # fight me
DEPS = csim.h
OBJ = csim.o main.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

csim: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o csim