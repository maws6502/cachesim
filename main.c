#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "csim.h"

char *rpols[] = {
    "lru",
    "nfu",
    "clock",
    "random"
};
#define NUM_RPOLS 4

int
main(int argc, char *argv[])
{
    /* attempt to stick to c89 */
    /* yeah like half of this is copypasta from the branch predictor */

    FILE *trace_file;
    Trace *trace_root, *t;
    struct sim_res sres;
    char trace_line[MAX_TRACE_LINE_LEN];
    int cs, bs, assoc, rpol, i;

    if (argc != 6 || access(argv[1], R_OK)) {
        goto error_out;
    }

    cs = strtol(argv[2], NULL, 10);
    if (errno || cs < 0) goto error_out;
    bs = strtol(argv[3], NULL, 10);
    if (errno || bs < 0) goto error_out;
    assoc = strtol(argv[4], NULL, 10);
    if (errno || assoc > 5 || assoc < 0) goto error_out;
    rpol = -1;
    for (i = 0; i < NUM_RPOLS; i++) {
        if(!strcmp(rpols[i], argv[5])) {
            rpol = RP_START + 1 + i;
            break;
        }
    }
    if (rpol == -1) goto error_out;
    
    trace_file = fopen(argv[1], "r");
    trace_root = malloc(sizeof(Trace)); /* this one's a throwaway, only for the next */
    t = trace_root;

    while (fgets(trace_line, MAX_TRACE_LINE_LEN, trace_file)) { /* never trust fscanf */
        char *tl;
        t->next = malloc(sizeof(Trace));
        tl = trace_line;
        while (*tl == ' ')
            tl++; /* trim leading whitespace */
        sscanf(tl, "0x%lx", &t->next->addr);
        t = t->next;
        t->next = NULL; /* you never know */
    }

    sres = csim(trace_root, cs, bs, assoc, rpol);

    printf("{\"accesses\": %lu, \"hits\": %lu, \"cold_misses\": %lu, \"conflict_misses\": %lu, \"capacity_misses\": %lu, \"cache_size\": %lu, \"block_size\": %lu, \"associativity\": %lu}",
        sres.accesses, sres.hits, sres.cold_misses, sres.conflict_misses, sres.capacity_misses, sres.cache_size, sres.block_size, sres.associativity);

    /* cleanup */

    t = trace_root;
    trace_root = trace_root->next;
    free(t);

    while(trace_root) {
        t = trace_root;
        trace_root = trace_root->next;
        free(t);
    }
    
    return 0;

    error_out:
    printf("Error: invalid arguments\n"); /* enough said */
    return 1;
}
