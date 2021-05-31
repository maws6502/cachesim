## EE318 Assignment 2

Aditya Goturu <aditya18203@mechyd.ac.in>

18XJ1A0203

### Compiling and running

To compile, run:

```
$ make
```

You will need a version of make, as well as a C compiler. The compiler is assumed to be Clang, specify otherwise in the CC argument:

```
$ make CC=gcc
```

To run the program, specify the trace file as the first argument:

```
$ ./csim </path/to/trace> <cs> <bs> <assoc> <rpol>
```

Cache parameters will be calculated as follows:

* `cache_size = 256 * 2^cs bytes` (a value of 0 will create a cache of 256 bytes) 
* `block_size = 4 * 2^bs bytes` (a value of 0 will create a block size of 4 bytes)
* `associativity = 2^assoc ways` (a value of 0 will create a direct mapped cache) 
* `rpol = {lru, clock, random}` (corrosponding replacement policy is chosen)

The program will run the trace through the given cache, and print output a valid JSON string containing the results of the run (except in the event of a failed assertion, error strrings are not JSON). There is no trailing newline after the JSON string.

#### Example

```
$ ./csim traces/crc32.trace 0 0 1 random                 
{"accesses": 9601024, "hits": 7768596, "cold_misses": 64, "conflict_misses": 1459823, "capacity_misses": 372541, "cache_size": 256, "block_size": 4, "associativity": 2}
```

#### All iterations

There also exists a program `csi` which iterates through a large range of parameters for a given trace file:

```
$ ./csi </path/to/trace>
```
`stdout` is the same as the order as the above program, but in csv form with trailing newlines after each iteration. Status is displayed on stderr. Order of status is `rpol`, `cs`, `bs`, `assoc`. `rpol` enumeration can be seen in `csim.h`.

##### Example `stderr`:
```
Now simulating: 1 0 0 0
Now simulating: 1 0 0 1
Now simulating: 1 0 0 2
Now simulating: 1 0 0 3
Now simulating: 1 0 0 4
Now simulating: 1 0 1 0
Now simulating: 1 0 1 1
Now simulating: 1 0 1 2
Now simulating: 1 0 1 3
Now simulating: 1 0 1 4
Now simulating: 1 0 2 0
Now simulating: 1 0 2 1
Now simulating: 1 0 2 2
```

### About

##### General program design
* Main program logic is in `csim.c` in the function `csim(Trace *t, uint64_t csize, uint64_t bsize, uint64_t assoc, int rpol)`. All parameters are passed in exactly as received from the command line, after sanitization.
* Variant of `main.c` in `csi.c`, automatically iterates through all parameters.
* Single header file (`csim.h`).
* Trace file is read into a linked list, and this list is passed onto `csim`. Structure of list is defined by `Trace {aka struct trace}` in `csim.h`.
* The `csim(...)` function ignores the root of the passed-in linked list, as only it's ->next is meaningful.

##### Code style
* C89 for the most part.
* Except for values which are to be printed, `stdint.h`'s explictly sized integer types are used. Printed values are `unsigned long`. Flags are `int`, as memory is cheap and native integer performance is fun. 