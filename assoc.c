/*  TAB-P
 *
 *  association list, using symbols as keys
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ASSOC_HASH_SIZE 16

#include "cell.h"
#include "cmod.h" // hash_undef

// TODO inline
cell *assoc_key(cell *cp) {
    assert(cp && (cp->type == c_KEYVAL));
    return cp->_.cons.car;
}

// TODO inline
cell *assoc_val(cell *cp) {
    assert(cp && (cp->type == c_KEYVAL));
    return cp->_.cons.cdr;
}

static INLINE cell **assoc_valp(cell *cp) {
    return &(cp->_.cons.cdr);
}

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
            if (assoc_key(p->_.cons.car) == key) { // exists already
                return 0;
            }
            p = p->_.cons.cdr;
        }
        // last item
        if (assoc_key(p) == key) { // exists already?
            return 0; 
        }
        // not found, make new entry
        *pp = cell_elist(cell_keyval(key, val), *pp);
    } else {
        // empty list, make new entry
        anode->_.assoc.table[hash] = cell_keyval(key, val);
    }
    return 1;
}

// as assoc_set, but allows redefine if hash_undef
int assoc_set_local(cell *anode, cell* key, cell* val) {
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
            if (assoc_key(p->_.cons.car) == key) { // exists already?
                if (assoc_val(p->_.cons.car) == hash_undef) {
                    cell_unref(hash_undef); // special case for undefined
                    *assoc_valp(p->_.cons.car) = val;
                    return 1;
                }
                return 0;
            }
            p = p->_.cons.cdr;
        }
        // last item
        if (assoc_key(p) == key) { // exists already?
            if (assoc_val(p) == hash_undef) {
                cell_unref(hash_undef); // special case for undefined
                *assoc_valp(p) = val;
                return 1;
            }
            return 0;
        }
        // not found, make new entry
        *pp = cell_elist(cell_keyval(key, val), *pp);
    } else {
        // empty list, make new entry
        anode->_.assoc.table[hash] = cell_keyval(key, val);
    }
    return 1;
}

// on success, key is consumed, but not anode
// return false if already defined, and do not consume key and val
int assoc_set_weak(cell *anode, cell* key, cell* val) {
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
            if (assoc_key(p->_.cons.car) == key) { // exists already?
                return 0;
            }
            p = p->_.cons.cdr;
        }
        // last item
        if (assoc_key(p) == key) { // exists already
            return 0;
        }
        // not found, make new entry
        *pp = cell_elist(cell_keyweak(key, val), *pp);
    } else {
        // empty list, make new entry
        anode->_.assoc.table[hash] = cell_keyweak(key, val);
    }
    return 1;
}

// neither key nor anode is consumed TODO evaluate
// is valuep is NILP, do not ref value
// false if not found
int assoc_get(cell *anode, cell* key, cell **valuep) {
    assert(cell_is_assoc(anode));

    if (anode->_.assoc.table != NULL) {
        cell *p;
	int hash = hash_cons(key);
        if ((p = anode->_.assoc.table[hash])) {
            while (cell_is_elist(p)) {
                if (assoc_key(p->_.cons.car) == key) { // found?
                    if (valuep) *valuep = cell_ref(assoc_val(p->_.cons.car));
                    return 1;
                }
                p = p->_.cons.cdr;
            }
            if (assoc_key(p) == key) { // found?
                if (valuep) *valuep = cell_ref(assoc_val(p));
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
    // in case this is in the middle of a gc
    if ((aa && aa->type==c_STOP) || (bb && bb->type==c_STOP)) return 0;

    // TODO what about non-symbolic keys
    assert(cell_is_symbol(assoc_key(aa)));
    assert(cell_is_symbol(assoc_key(aa)));
    return strcmp(assoc_key(aa)->_.symbol.nam, assoc_key(bb)->_.symbol.nam);
}

void assoc_iter(struct assoc_i *ip, cell *anode) {
    assert(cell_is_assoc(anode));
    ip->anode = anode;
    ip->sorted = NILP;
    ip->p = NIL;
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
            ip->sorted = NILP;
        }
        return result;
    }

    while (!ip->p) {
        if (ip->h >= ASSOC_HASH_SIZE) {
            return NIL;
        }
        if (ip->anode->_.assoc.table == NULL) {
            return NIL; // TODO do in initializer?
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
    // result should be a c_KEYVAL or c_STOP in gc
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
