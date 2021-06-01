/*  TAB-P
 *
 *  allocate cells in chunks
 */

#include <stdio.h> // fprintf
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cell.h"

#define CHUNK_SIZE 1000 // number of cells in a chunk

struct chunk_s {
    struct chunk_s *next;
    cell nodes[CHUNK_SIZE];
};

// holds all free cells
static cell *freelist = NIL;

// holds all chunks
static struct chunk_s *chunks = (struct chunk_s *)0;

// need a new chunk
cell *newchunk() {
    int i;
    struct chunk_s *cp = malloc(sizeof(struct chunk_s ));
    assert(cp);
    memset(cp, 0, sizeof(struct chunk_s));
    // add all but the last one to freelist
    for (i = 0; i < CHUNK_SIZE-1; ++i) {
        cp->nodes[i].type = c_FREE;
        cp->nodes[i]._.cons.car = freelist;
        freelist = &(cp->nodes[i]);
    }
    cp->next = chunks;
    chunks = cp;

    return &(cp->nodes[CHUNK_SIZE-1]);
}

cell *newnode(celltype t) {
    cell *node;
    if (freelist) {
        node = freelist;
        assert(node->type == c_FREE);
        freelist = node->_.cons.car;
        node->_.cons.car = NIL;
    } else {
        node = newchunk();
    }
    node->type = t;
    node->ref = 1;
    return node;
}

// TODO inline
void freenode(cell *node) {
    node->type = c_FREE;
    node->_.cons.car = freelist;
    node->_.cons.cdr = NIL; // not required
    freelist = node->_.cons.car;
}

void node_exit() {
    int i;
    integer_t nonfree = 0;
    struct chunk_s *cp;
    for (cp = chunks; cp; cp = cp->next) {
        for (i = 0; i < CHUNK_SIZE; ++i) {
            if (cp->nodes[i].type != c_FREE) {
                // should not happen, cell is not released
                // TODO option for this...
                if (nonfree < 10) {
                    fflush(stdout);
                    fprintf(stderr,"error; cell @0x%llx type %d ref %d not free\n",
                    (unsigned long long)&(cp->nodes[i]), cp->nodes[i].type, cp->nodes[i].ref);
                }
                ++nonfree;
            }
        }
    }
    if (nonfree == 0) {
        freelist = NIL;
        while (chunks) {
            cp = chunks;
            chunks = chunks->next;
            free(cp);
        }
    }
}

void node_init() {
    freelist = NIL;
}
