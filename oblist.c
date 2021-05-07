/*  TAB P
 *
 *  TODO should all symbols have values?
 *  Or only symbols that have been defined? Probably
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "oblist.h"

// add symbol to oblist
// assume it is malloc()-ed
// if it exists already, free()

#define OBLIST_HASH_SIZE 256

struct ob_entry {
    struct ob_entry *next;
    cell namdef; // TODO can also be a pointer, TBD
};

static struct ob_entry* ob_table[OBLIST_HASH_SIZE];

static unsigned int oblist_hash(const char *sym) {
    unsigned int hash = 0;
    assert(sym);
    while (*sym) {
	hash = (hash >> 10) ^ (hash << 3) ^ *sym++;
    }
    return hash % OBLIST_HASH_SIZE;
}

cell *oblist(char *sym) {
    return oblista(strdup(sym));
}

// assume symbol is malloc()'d
cell *oblista(char *sym) {
    struct ob_entry **pp;
    int hash = oblist_hash(sym);
    pp = &(ob_table[hash]);
    while (*pp) {
        if (strcmp((*pp)->namdef._.symbol.nam, sym) == 0) {
	    // exists already
	    free(sym);
            return &((*pp)->namdef);
	}
	pp = &(*pp)->next;
    }
    // not found, make entry
    *pp = malloc(sizeof(struct ob_entry));
    (*pp)->next = 0;
    (*pp)->namdef.type = c_SYMBOL;
    (*pp)->namdef.ref = 1;
    (*pp)->namdef._.symbol.val = NIL; // TODO should probably be #void instead
    (*pp)->namdef._.symbol.nam = sym;
    return &((*pp)->namdef);
}

// clean up on exit
void oblist_drop() {
    int h;
    struct ob_entry *p;
    printf("\n\noblist:\n");
    for (h = 0; h < OBLIST_HASH_SIZE; ++h) {
	p = ob_table[h];
	ob_table[h] = 0;
	while (p) {
	    struct ob_entry *q = p->next;
	    printf("%s\n", p->namdef._.symbol.nam);
	    cell_unref(p->namdef._.symbol.val);
            free(p->namdef._.symbol.nam);
	    free(p);
	    p = q;
	}
    }
    printf("\n");
}

