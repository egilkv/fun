/*  TAB P
 *
 *  TODO should all symbols have values?
 *  Or only symbols that have been defined? Probably
 */

#include <stdio.h> // TODO debug only
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "oblist.h"

#define OBLIST_HASH_SIZE 256

struct ob_entry {
    struct ob_entry *next;
    cell namdef; // TODO can also be a pointer, TBD
};

static struct ob_entry* ob_table[OBLIST_HASH_SIZE];

static unsigned int hash_string(const char *sym) {
    unsigned int hash = 0;
    assert(sym);
    while (*sym) {
	hash = (hash >> 10) ^ (hash << 3) ^ *sym++;
    }
    return hash % OBLIST_HASH_SIZE;
}

// add symbol to oblist, with value
// symbol is alloc'd as needed
// val is consumed
cell *oblistv(const char *sym, cell *val) {
    cell *ob = oblista(strdup(sym));
    oblist_set(ob, val);
    return ob;
}

// find or create symbol
// assume symbol is malloc()'d
// return unreffed symbol
cell *oblista(char *sym) {
    struct ob_entry **pp;
    int hash = hash_string(sym);
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
    // TODO if created, state should be "not yet defined"
    (*pp)->namdef._.symbol.val = NIL;
    (*pp)->namdef._.symbol.nam = sym;
    return &((*pp)->namdef);
}

// find symbol, or create as undef as needed
// symbol will be malloc()'d
// TODO if created, state should be "not yet defined"
// return unreffed symbol
cell *oblists(const char *sym) {
    // TODO combine with oblista() above
    struct ob_entry **pp;
    int hash = hash_string(sym);
    pp = &(ob_table[hash]);
    while (*pp) {
        if (strcmp((*pp)->namdef._.symbol.nam, sym) == 0) {
	    // exists already
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
    (*pp)->namdef._.symbol.nam = strdup(sym);
    return &((*pp)->namdef);
}

// define value of variable
// val is consumed
void oblist_set(cell *sym, cell *val) {
    assert(cell_is_symbol(sym));
    cell_unref(sym->_.symbol.val);
    sym->_.symbol.val = val;
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

