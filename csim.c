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
    CacheIndex *cache;
    FA *faroot, *f, *r2, *r3, *faend;
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

    faroot = calloc(1, sizeof(FA));
    f = faroot;
    for (i = 0; i < csize/bsize; i++) {
        f->next = calloc(1, sizeof(FA));
        f->next->prev = f;
        f->next->next = NULL;
        f = f->next;
    }
    faend = f;

    /* then, generate the actual cache */
    cache = calloc(indexes, sizeof(CacheIndex));
    for (i = 0; i < indexes; i++) {
        cache[i].slots = calloc(assoc, sizeof(CacheLine));
        cache[i].uc = calloc(assoc, sizeof(uint64_t));
        if (rpol == RP_LRU) {
            for (j = 0; j < assoc; j++)
                cache[i].uc[j] = j;
        }
    }

    /* end of setup, run actual sim */

    while(t = t->next) {
        res.accesses++;
        /* before anything else, we trim the lsbs down to the block size */
        tag = t->addr >> tshift;

        /* first, check FA LRU, and update it */
        /* step 1: search. linked list things i think */
        fa_hit = 0;
        f = faroot;

        while (f = f->next) {
            if (f->tag == tag) {
                fa_hit = 1;
                if (f == faroot->next) break;
                r2 = f->prev;
                r3 = f->next;

                faroot->next->prev = f;
                f->next = faroot->next;
                faroot->next = f;
                
                r2->next = r3;
                if(r3)
                    r3->prev = r2;
                else
                    faend = r2;
                
                break;
            }
        }

        if (!fa_hit) {
            /* free the end */
            faend = faend->prev;
            free(faend->next);
            faend->next = NULL;
            /* splice in our new boi at the beginning */
            r2 = calloc(1, sizeof(FA));
            r2->next = faroot->next;
            r2->prev = faroot;
            r2->tag = tag;
            faroot->next->prev = r2;
            faroot->next = r2;
        }


        /* step 2: check our actual cache */
        mc_hit = 0;
        mcindex = tag & imask;
        for (i = 0; i < assoc; i++) {
            if (cache[mcindex].slots[i].valid && 
                    cache[mcindex].slots[i].tag == tag) {
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
                if(cache[mcindex].uc[j] > cache[mcindex].uc[i])
                    cache[mcindex].uc[j]--;
                }
                cache[mcindex].uc[i] = assoc - 1;
                break;
            case RP_CLK:
                cache[mcindex].uc[i] = 1; /* give it a chance */
            case RP_RND:
                /* monke */
                break;
            }
        } else {
            switch (rpol) {
            case RP_LRU:
                for (j = 0; j < assoc; j++) {
                    if (cache[mcindex].uc[j]) {
                        cache[mcindex].uc[j]--;
                    } else {
                        cache[mcindex].uc[j] = assoc - 1;
                        cache[mcindex].slots[j].tag = tag;
                        if (cache[mcindex].slots[j].valid) {
                            if (fa_hit)
                                res.conflict_misses++;
                            else
                                res.capacity_misses++;
                        } else {
                            res.cold_misses++;
                        }
                        cache[mcindex].slots[j].valid = 1;
                    }
                }
                break;

            case RP_CLK:
                /* first ensure that we don't have any cold slots */
                inv_available = 0;

                for (j = 0; j < assoc; j++) {
                    if (!cache[mcindex].slots[j].valid) {
                        res.cold_misses++;
                        inv_available = 1;
                        break;
                    }
                }

                /* if invalid slots aren't available, we look for a slot without a sc bit */
                if (!inv_available) {
                    for (j = 0; j < assoc; j++) {
                        if (cache[mcindex].uc[j]) cache[mcindex].uc[j] = 0;
                        else break;
                    }

                    if (fa_hit)
                        res.conflict_misses++;
                    else
                        res.capacity_misses++;
                }

                
                cache[mcindex].slots[j].tag = tag;
                cache[mcindex].slots[j].valid = 1;

                break;

            case RP_RND:
                /* first ensure that we don't have any cold slots */
                inv_available = 0;

                for (j = 0; j < assoc; j++) {
                    if (!cache[mcindex].slots[j].valid) {
                        res.cold_misses++;
                        inv_available = 1;
                        break;
                    }
                }

                /* if invalid slots aren't available, nuke a random slot */
                if (!inv_available) {
                    j = rand() % assoc;
                    if (fa_hit)
                        res.conflict_misses++;
                    else
                        res.capacity_misses++;
                }

                cache[mcindex].slots[j].valid = 1;
                cache[mcindex].slots[j].tag = tag;
                break;
            }
        }
    }

    /* cleanup */

    for (i = 0; i < indexes; i++) {
        free(cache[i].slots);
        free(cache[i].uc);
    }
    free(cache);

    res.block_size = bsize;
    res.cache_size = csize;
    res.associativity = assoc;

    return res;
}