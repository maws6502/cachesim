#include <stdlib.h>
#include <assert.h>

#include "csim.h"

struct sim_res
csim(Trace *t, uint64_t csize, uint64_t bsize, uint64_t assoc, int rpol)
{
    /* simulate the given cache and a fully associative LRU cache
     * FA LRU is used for differentiating between conflict and capacity misses */
    uint64_t i, j, indexes, index_size, imask, tshift, tag, mcindex;
    int fa_hit, mc_hit, inv_available;
    CacheIndex *maincache, *facache;
    struct sim_res res = {0, 0, 0, 0, 0};

    /* shift cache sizes to become real sizes
     * a value of 0 indicates minimum size (csize 256 bytes, bsize 4 bytes).
     * assoc minimum 1 */

    assoc = 1L << assoc;
    tshift = 2 + bsize;

    csize = 1L << (8 + csize);
    bsize = 1L << (2 + bsize);

    index_size = bsize * assoc;
    indexes = csize/index_size;
    imask = indexes - 1L;

    assert(csize > index_size);

    /* first, generate a fully associative cache */
    facache = calloc(csize/bsize, sizeof(CacheIndex));
    for (i = 0; i < csize/bsize; i++) {
        facache[i].slots = calloc(1, sizeof(CacheLine));
        facache[i].uc = calloc(1, sizeof(uint64_t));
        facache[i].uc[0] = i;
    }

    /* then, generate the actual cache */
    maincache = calloc(indexes, sizeof(CacheIndex));
    for (i = 0; i < indexes; i++) {
        maincache[i].slots = calloc(assoc, sizeof(CacheLine));
        maincache[i].uc = calloc(assoc, sizeof(uint64_t));
        for (j = 0; j < assoc; j++)
            maincache[i].uc[j] = j;
    }

    /* end of setup, run actual sim */

    while(t = t->next) {
        res.accesses++;
        /* before anything else, we trim the lsbs down to the block size */
        tag = t->addr >> tshift;

        /* first, check FA LRU, and update it */
        /* step 1: search. if hit is also MRU, no need to LRupdate */
        fa_hit = 0;
        for (i = 0; i < csize/bsize; i++) {
            if (facache[i].slots[0].valid && facache[i].slots[0].tag == tag) {
                fa_hit = 1;
                break;
            }
        }
        /* on hit, fa_hit = 1 and i contains the index of the hit
         * otherrwise, fa_hit = 0 
         * in event of a hit, we need to decrement all counters
         * that are greater than the counter we have on index
         * then update the index to the highest value
         * if event of a miss, we need to decrement all nonzero values
         * and then add our tag to the zero index, and update it to MRU
         */
        if (fa_hit) {
            for (j = 0; j < csize/bsize; j++) {
                if(facache[j].uc[0] > facache[i].uc[0])
                    facache[j].uc[0]--;
            }
            facache[i].uc[0] = csize/bsize - 1;
        } else {
            for (j = 0; j < csize/bsize; j++) {
                if (facache[j].uc[0]) {
                    facache[j].uc[0]--;
                } else {
                    facache[j].uc[0] = csize/bsize - 1;
                    facache[j].slots[0].valid = 1;
                    facache[j].slots[0].tag = tag;
                }
            }
        }

        /* step 2: check our actual cache */
        mc_hit = 0;
        mcindex = tag & imask;
        for (i = 0; i < assoc; i++) {
            if (maincache[mcindex].slots[i].valid && 
                    maincache[mcindex].slots[i].tag == tag) {
                mc_hit = 1;
                break;
            }
        }

        /* at this point i contains the associativity index of the hit if mc_hit = 1
         * otherwise it is a miss
         * in the event of a hit, we have to update the usage counters based on the rpol
         * in the event of a miss, first we check which miss it is, then choose 
         * which slot to eject, write the new version, then update any counters on rpol
         */
        if (mc_hit) {
            res.hits++;
            switch (rpol) {
            case RP_LRU:
                for (j = 0; j < assoc; j++) {
                if(maincache[mcindex].uc[j] > maincache[mcindex].uc[i])
                    maincache[mcindex].uc[j]--;
                }
                maincache[mcindex].uc[i] = assoc - 1;
                break;
            case RP_RND:
                /* monke */
                break;
            }
        } else {
            switch (rpol) {
            case RP_LRU:
                for (j = 0; j < assoc; j++) {
                    if (maincache[mcindex].uc[j]) {
                        maincache[mcindex].uc[j]--;
                    } else {
                        maincache[mcindex].uc[j] = assoc - 1;
                        maincache[mcindex].slots[j].tag = tag;
                        if (maincache[mcindex].slots[j].valid) {
                            if (fa_hit)
                                res.capacity_misses++;
                            else
                                res.conflict_misses++;
                        } else {
                            res.cold_misses++;
                        }
                        maincache[mcindex].slots[j].valid = 1;
                    }
                }
                break;
            case RP_RND:
                /* first ensure that we don't have any cold slots */
                inv_available = 0;

                for (j = 0; j < assoc; j++) {
                    if (!maincache[mcindex].slots[j].valid) {
                        res.cold_misses++;
                        inv_available = 1;
                        break;
                    }
                }

                if (!inv_available) {
                    j = rand() % assoc;
                    if (fa_hit)
                        res.capacity_misses++;
                    else
                        res.conflict_misses++;
                }

                maincache[mcindex].slots[j].valid = 1;
                maincache[mcindex].slots[j].tag = tag;
                break;
            }
        }
    }

    /* cleanup */

    for (i = 0; i < csize/bsize; i++) {
        free(facache[i].slots);
        free(facache[i].uc);
    }
    free(facache);
    for (i = 0; i < indexes; i++) {
        free(maincache[i].slots);
        free(maincache[i].uc);
    }
    free(maincache);

    res.block_size = bsize;
    res.cache_size = csize;
    res.associativity = assoc;

    return res;
}