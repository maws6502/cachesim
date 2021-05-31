/* EE318 Assignment 2
 * Aditya Goturu <aditya18203@mechyd.ac.in>
 * 18XJ1A0203
 * Moriya Prateek Velagaleti <prateek18224@mechyd.ac.in>
 * 18XJ1A0224
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "csim.h"


int
main(int argc, char *argv[])
{
    /* attempt to stick to c89 */
    /* yeah like half of this is copypasta from the branch predictor */

    FILE *trace_file;
    Trace *trace_root, *t;
    struct sim_res sres;
    char trace_line[MAX_TRACE_LINE_LEN];
    int cs, bs, assoc, rpol;

    if (argc != 2 || access(argv[1], R_OK)) {
        goto error_out;
    }

    srand(LIKE_70_OR_SOMETHING);

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

    fclose(trace_file);

    for (rpol = RP_LRU; rpol <= RP_RND; rpol++)
    for (cs = 0; cs <= 14; cs++)
    for (bs = 0; bs <= 7; bs++)
    for (assoc = 0; assoc <= 4; assoc++)
    if (256 * (1L<<cs) > (1L<<assoc) * (4 * (1L<<bs))) {
        fprintf(stderr, "Now simulating: %d %d %d %d\n", rpol, cs, bs, assoc);
        fflush(stderr);
        sres = csim(trace_root, cs, bs, assoc, rpol);
        printf("%d %lu %lu %lu %lu %lu %lu %lu %lu\n", sres.replacement_policy, sres.cache_size,
            sres.block_size, sres.associativity, sres.accesses, 
            sres.hits, sres.cold_misses, sres.conflict_misses, sres.capacity_misses);
        fflush(stdout);
    }

    
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