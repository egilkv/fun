/*  TAB P
 *
 *  association list, using symbols as keys
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ASSOC_HASH_SIZE 16

#include "cell.h"

static unsigned int assoc_hash(cell *key) {
    // TODO 64bit
    // TODO improve...
    return ((((unsigned long)key) >> 3)
          ^ (((unsigned long)key) >> 10)) % ASSOC_HASH_SIZE;
}

// key and val are consumed, but not anode TODO
void assoc_set(cell *anode, cell* key, cell* val) {
    struct assoc_s **pp;
    int hash = assoc_hash(key);
    assert(anode->type == c_ASSOC);

    if (anode->_.assoc.table == NULL) {
        // table not yet allocated
	anode->_.assoc.table = malloc(ASSOC_HASH_SIZE * sizeof(struct assoc_s *));
	assert(anode->_.assoc.table);
	memset(anode->_.assoc.table, 0, ASSOC_HASH_SIZE * sizeof(struct assoc_s *));
    }

    pp = &(anode->_.assoc.table[hash]);
    while (*pp) {
	if ((*pp)->key == key) {
	    // exists already TODO should not be allowed
	    free((*pp)->val);
	    (*pp)->val = val;
            cell_unref(key);
	    return;
	}
	pp = &(*pp)->next;
    }
    // not found, make entry
    *pp = malloc(sizeof(struct assoc_s));
    (*pp)->next = 0;
    (*pp)->key = key;
    (*pp)->val = val;
}

// neither key nor anode is consumed TODO evaluate
// false if not found
int assoc_get(cell *anode, cell* key, cell **valuep) {
    assert(anode->type == c_ASSOC);

    if (anode->_.assoc.table != NULL) {
	struct assoc_s **pp;
	int hash = assoc_hash(key);
	pp = &(anode->_.assoc.table[hash]);
	while (*pp) {
	    if ((*pp)->key == key) {
		*valuep = cell_ref((*pp)->val);
		return 1;
	    }
	    pp = &(*pp)->next;
	}
    }
    // not found
    return 0;
}

// clean up on exit
void assoc_drop(cell *anode) {
    assert(anode->type == c_ASSOC);
    if (anode->_.assoc.table) {
        struct assoc_s *p;
        int h;
        for (h = 0; h < ASSOC_HASH_SIZE; ++h) {
            p = anode->_.assoc.table[h];
            while (p) {
                struct assoc_s *q = p->next;
                cell_unref(p->key);
                cell_unref(p->val);
                free(p);
                p = q;
            }
        }
        free(anode->_.assoc.table);
        anode->_.assoc.table = NULL; // not really required
    }
}

