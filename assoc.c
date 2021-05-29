/*  TAB-P
 *
 *  association list, using symbols as keys
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ASSOC_HASH_SIZE 16

#include "cell.h"

static unsigned int hash_cons(cell *key) {
    // TODO 64bit
    // TODO improve...
    return ((((unsigned long)key) >> 3)
          ^ (((unsigned long)key) >> 10)) % ASSOC_HASH_SIZE;
}

// on success, key and val are consumed, but not anode
// return false if already defined, and do not consume key and val
int assoc_set(cell *anode, cell* key, cell* val) {
    cell *p;
    cell **pp;
    int hash = hash_cons(key);
    assert(cell_is_assoc(anode));

    if (anode->_.assoc.table == NULL) {
        // table not yet allocated
	anode->_.assoc.table = malloc(ASSOC_HASH_SIZE * sizeof(struct assoc_s *));
	assert(anode->_.assoc.table);
	memset(anode->_.assoc.table, 0, ASSOC_HASH_SIZE * sizeof(struct assoc_s *));
	anode->_.assoc.size = ASSOC_HASH_SIZE;
    }
    pp = &(anode->_.assoc.table[hash]);

    if ((p = *pp)) { // there is a table?
        while (cell_is_elist(p)) {
            assert(cell_is_pair(p->_.cons.car));
            if (p->_.cons.car->_.cons.car == key) {
                // exists already
                return 0;
            }
            p = p->_.cons.cdr;
        }
        // last item
        assert(cell_is_pair(p));
        if (p->_.cons.car == key) {
            return 0; // exists already
        }
        // not found, make new entry
        *pp = cell_elist(cell_pair(key,val), *pp);
    } else {
        // empty list, make new entry
        anode->_.assoc.table[hash] = cell_pair(key,val);
    }
    return 1;
}

// neither key nor anode is consumed TODO evaluate
// false if not found
int assoc_get(cell *anode, cell* key, cell **valuep) {
    assert(cell_is_assoc(anode));

    if (anode->_.assoc.table != NULL) {
        cell *p;
	int hash = hash_cons(key);
        if ((p = anode->_.assoc.table[hash])) {
            while (cell_is_elist(p)) {
                assert(cell_is_pair(p->_.cons.car));
                if (p->_.cons.car->_.cons.car == key) { // found?
                    *valuep = cell_ref(p->_.cons.car->_.cons.cdr);
                    return 1;
                }
                p = p->_.cons.cdr;
            }
            assert(cell_is_pair(p));
            if (p->_.cons.car == key) { // found?
                *valuep = cell_ref(p->_.cons.cdr);
                return 1;
            }
	}
    }
    // not found
    return 0;
}

static int compar_sym(const void *a, const void *b) {
    cell *aa = *((cell **)a);
    cell *bb = *((cell **)b);
    // TODO what about non-symbolic keys
    assert(cell_is_symbol(cell_car(aa)));
    assert(cell_is_symbol(cell_car(aa)));
    return strcmp(cell_car(aa)->_.symbol.nam, cell_car(bb)->_.symbol.nam);
}

void assoc_iter(struct assoc_i *ip, cell *anode) {
    assert(cell_is_assoc(anode));
    ip->anode = anode;
    ip->sorted = NULL;
    ip->p = NULL;
    ip->h = 0;
}

// iterate over assoc, return key-val pair
// TODO inline?
struct cell_s *assoc_next(struct assoc_i *ip) {
    struct cell_s *result;
    if (ip->sorted) {
        result = ip->sorted[(ip->h)++];
        if (!result) { // end of list
            free(ip->sorted);
            ip->sorted = NULL;
        }
        return result;
    }

    while (!ip->p) {
        if (ip->h >= ASSOC_HASH_SIZE) {
            return NULL;
        }
        if (ip->anode->_.assoc.table == NULL) {
            return NULL; // TODO do in initializer?
        }
        ip->p = ip->anode->_.assoc.table[ip->h];
        (ip->h)++;
    }
    if (cell_is_elist(ip->p)) {
        result = ip->p->_.cons.car;
        ip->p = ip->p->_.cons.cdr;
    } else {
        result = ip->p;
        ip->p = NIL;
    }
    assert(cell_is_pair(result));
    return result;
}

// iterate, returning key-val pairs in sorted sequence
void assoc_iter_sorted(struct assoc_i *ip, cell *anode) {
    cell *p;
    index_t length = 0;
    cell **vector = malloc(sizeof(cell *));
    assert(vector);
    assoc_iter(ip, anode);

    while ((p = assoc_next(ip))) {
        vector = realloc(vector, (++length+1) * sizeof(cell *));
        assert(vector);
        vector[length-1] = p;
    }
    vector[length] = NULL;
    if (length > 1) {
        qsort(vector, length, sizeof(struct assoc_s *), compar_sym);
    }
    ip->sorted = vector;
    ip->h = 0;
}

// clean up on unref, all but node itself
void assoc_drop(cell *anode) {
    cell **t;
    assert(cell_is_assoc(anode));

    if ((t = anode->_.assoc.table)) {
        index_t h;
        anode->_.assoc.table = NULL;
        for (h = 0; h < ASSOC_HASH_SIZE; ++h) {
            cell_unref(t[h]);
        }
        free(t);
    }
}

