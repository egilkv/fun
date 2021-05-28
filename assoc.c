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
    struct assoc_s **pp;
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
    while (*pp) {
	if ((*pp)->key == key) {
	    // exists already
	    // cell_unref((*pp)->val);
	    // (*pp)->val = val;
	    // cell_unref(key);
	    return 0;
	}
	pp = &(*pp)->next;
    }
    // not found, make entry
    *pp = malloc(sizeof(struct assoc_s));
    (*pp)->next = 0;
    (*pp)->key = key;
    (*pp)->val = val;
    return 1;
}

// neither key nor anode is consumed TODO evaluate
// false if not found
int assoc_get(cell *anode, cell* key, cell **valuep) {
    assert(cell_is_assoc(anode));

    if (anode->_.assoc.table != NULL) {
	struct assoc_s **pp;
	int hash = hash_cons(key);
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

static int compar_sym(const void *a, const void *b) {
    struct assoc_s *aa = *((struct assoc_s **)a);
    struct assoc_s *bb = *((struct assoc_s **)b);
    // TODO what about non-symbolic keys
    assert(cell_is_symbol(aa->key));
    assert(cell_is_symbol(bb->key));
    return strcmp(aa->key->_.symbol.nam, bb->key->_.symbol.nam);
}

void assoc_iter(struct assoc_i *ip, cell *anode) {
    assert(cell_is_assoc(anode));
    ip->anode = anode;
    ip->p = NULL;
    ip->h = 0;
}

// iterate over assoc
// TODO inline?
struct assoc_s *assoc_next(struct assoc_i *ip) {
    struct assoc_s *result;
    while (!ip->p) {
        if (ip->h >= ASSOC_HASH_SIZE) {
            return NULL;
        }
        if (ip->anode->_.assoc.table == NULL) {
            return NULL; // TODO do in initializer
        }
        ip->p = (ip->anode)->_.assoc.table[ip->h];
        (ip->h)++;
    }
    result = ip->p;
    ip->p = ip->p->next;
    return result;
}

 // return all assoc key pairs as an allocated NULL-terminated vector
// TODO slow
struct assoc_s **assoc2vector(cell *anode) {
    struct assoc_i iter;
    struct assoc_s *p;
    struct assoc_s **vector = malloc(sizeof(struct assoc_s *));
    index_t length = 0;
    assert(vector);
    assoc_iter(&iter, anode);

    while ((p = assoc_next(&iter))) {
        vector = realloc(vector, (++length+1) * sizeof(struct assoc_s *));
        assert(vector);
        vector[length-1] = p;
    }
    vector[length] = NULL;
    if (length > 1) {
        qsort(vector, length, sizeof(struct assoc_s *), compar_sym);
    }
    return vector;
}

// clean up on unref
void assoc_drop(cell *anode) {
    struct assoc_i iter;
    struct assoc_s *p;
    assoc_iter(&iter, anode);

    while ((p = assoc_next(&iter))) {
        cell_unref(p->key);
        cell_unref(p->val);
        free(p);
    }
}

