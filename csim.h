
#ifndef _STDINT_H
#include <stdint.h>
#endif
#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#ifndef _CSIM_H
#define _CSIM_H 1


#define MAX_TRACE_LINE_LEN 40 /* seems familiar */

enum ReplacementPolicy {
    RP_START,
    RP_LRU,
    RP_NFU,
    RP_CLK,
    RP_RND,
    RP_END
};

struct cache_line {
    uint64_t tag;
    char valid;
};

typedef struct cache_line CacheLine;

struct cache_index {
    CacheLine *slots;
    uint64_t *uc;
};

typedef struct cache_index CacheIndex;

struct trace {
        uint64_t addr;     /* assuming all the trace files are 64 bit addresses at max */
        struct trace *next;     /* linked list, seemed appropriate somehow */
};

typedef struct trace Trace;

struct sim_res {
    unsigned long accesses;
    unsigned long hits;
    unsigned long cold_misses;
    unsigned long conflict_misses;
    unsigned long capacity_misses;
    unsigned long cache_size;
    unsigned long block_size;
    unsigned long associativity;
};

struct sim_res      csim        (Trace *,       uint64_t,      uint64_t,      uint64_t,      int     );

#endif