/* TAB-P
 *                                                // TODO superflous
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>                               // TODO superflous

#include "oblist.h"

static  cell *newcell(celltype t) {
    cell *node = malloc(sizeof(cell));
    assert(node);
    memset(node, 0, sizeof(cell)); // TODO improve
    node->type = t;
    node->ref = 1;
    return node;
}

cell *cell_list(cell *car, cell *cdr) {
    cell *node = newcell(c_LIST);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

cell *cell_func(cell *car, cell *cdr) {
    cell *node = newcell(c_FUNC);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

cell *cell_pair(cell *car, cell *cdr) {
    cell *node = newcell(c_PAIR);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

// TODO inline
cell *cell_ref(cell *cp) {
    if (cp) ++(cp->ref);
    return cp;
}

// TODO inline
int cell_is_list(cell *cp) {
    return cp && cp->type == c_LIST;
}

int cell_is_func(cell *cp) {
    return cp && cp->type == c_FUNC;
}

int cell_is_pair(cell *cp) {
    return cp && cp->type == c_PAIR;
}

// TODO inline
int cell_is_vector(cell *cp) {
    return cp && cp->type == c_VECTOR;
}

// TODO inline
int cell_is_symbol(cell *cp) {
    return cp && cp->type == c_SYMBOL;
}

// TODO inline
int cell_is_string(cell *cp) {
    return cp && cp->type == c_STRING;
}

// TODO inline
int cell_is_assoc(cell *cp) {
    return cp && cp->type == c_ASSOC;
}

int cell_is_special(cell *cp, const char *magic) {
    return cp && cp->type == c_SPECIAL && (!magic || cp->_.special.magic == magic);
}

// TODO inline
int cell_is_integer(cell *cp) {
    return cp && cp->type == c_INTEGER;
}

cell *cell_car(cell *cp) {
    assert(cp && (cp->type == c_LIST || cp->type == c_FUNC || cp->type == c_PAIR));
    return cp->_.cons.car;
}

cell *cell_cdr(cell *cp) {
    assert(cp && (cp->type == c_LIST || cp->type == c_FUNC || cp->type == c_PAIR));
    return cp->_.cons.cdr;
}

int list_split(cell *cp, cell **carp, cell **cdrp) {
    if (cell_is_list(cp)) {
        if (carp) *carp = cell_ref(cp->_.cons.car);
        if (cdrp) *cdrp = cell_ref(cp->_.cons.cdr);
        cell_unref(cp);
        return 1;
     } else {
        *carp = NIL;
        *cdrp = NIL;
        return 0;
     }
}

int func_split(cell *cp, cell **carp, cell **cdrp) {
    if (cell_is_func(cp)) {
        if (carp) *carp = cell_ref(cp->_.cons.car);
        if (cdrp) *cdrp = cell_ref(cp->_.cons.cdr);
        cell_unref(cp);
        return 1;
     } else {
        *carp = NIL;
        *cdrp = NIL;
        return 0;
     }
}

int pair_split(cell *cp, cell **carp, cell **cdrp) {
    if (cell_is_pair(cp)) {
        if (carp) *carp = cell_ref(cp->_.cons.car);
        if (cdrp) *cdrp = cell_ref(cp->_.cons.cdr);
        cell_unref(cp);
        return 1;
     } else {
        *carp = NIL;
        *cdrp = NIL;
        return 0;
     }
}

// symbol that is not malloc'd
// TODO should optimze wrt constant symbols
cell *cell_symbol(const char *symbol) {
    return cell_asymbol(strdup(symbol));
}

// symbol that is malloc'd already
cell *cell_asymbol(char *symbol) {
    return cell_ref(oblista(symbol));
}

cell *cell_astring(char_t *string, index_t length) {
    cell *node = newcell(c_STRING);
    node->_.string.ptr = string;
    node->_.string.len = length;
    return node;
}

cell *cell_integer(integer_t integer) {
    cell *node = newcell(c_INTEGER);
    node->_.ivalue = integer;
    return node;
}

cell *cell_cfunQ(struct cell_s *(*fun)(struct cell_s *, struct env_s *)) {
    cell *node = newcell(c_CFUNQ);
    node->_.cfunq.def = fun;
    return node;
}

cell *cell_cfunN(struct cell_s *(*fun)(struct cell_s *)) {
    cell *node = newcell(c_CFUNN);
    node->_.cfun1.def = fun;
    return node;
}

cell *cell_cfun0(struct cell_s *(*fun)(void)) {
    cell *node = newcell(c_CFUN0);
    node->_.cfun0.def = fun;
    return node;
}

cell *cell_cfun1(struct cell_s *(*fun)(struct cell_s *)) {
    cell *node = newcell(c_CFUN1);
    node->_.cfun1.def = fun;
    return node;
}

cell *cell_cfun2(struct cell_s *(*fun)(struct cell_s *, struct cell_s *)) {
    cell *node = newcell(c_CFUN2);
    node->_.cfun2.def = fun;
    return node;
}

cell *cell_cfun3(struct cell_s *(*fun)(struct cell_s *, struct cell_s *, struct cell_s *)) {
    cell *node = newcell(c_CFUN3);
    node->_.cfun3.def = fun;
    return node;
}

cell *cell_vector(index_t length) {
    // TODO or NIL if length==0
    cell *node = newcell(c_VECTOR);
    node->_.vector.len = length;
    if (length > 0) { // if length==0 table is NULL
        // TODO have some sanity check on vector length
        node->_.vector.table = malloc(length * sizeof(cell *));
        assert(node->_.vector.table);
        memset(node->_.vector.table, 0, length * sizeof(cell *));
    }
    return node;
}

void vector_resize(cell* node, index_t newlen) {
    // TODO have some sanity check on new vector length
    index_t oldlen;
    assert(cell_is_vector(node));
    oldlen = node->_.vector.len;

    if (oldlen > 0 && oldlen > newlen) {
        index_t i;
	for (i = newlen; i < oldlen; ++i) {
	    cell_unref(node->_.vector.table[i]);
	    node->_.vector.table[i] = NIL; // TODO not really needed
	}
    }
    if (newlen > 0 && newlen != oldlen) {
	// realloc works also with NULL
        node->_.vector.table = realloc(node->_.vector.table, newlen * sizeof(cell *));
	assert(node->_.vector.table);
	if (newlen > oldlen) {
	    memset(&(node->_.vector.table[oldlen]), 0, (newlen-oldlen) * sizeof(cell *));
	}

    } else if (oldlen > 0) {
	free(node->_.vector.table);
	node->_.vector.table = NULL;
    } else {
	assert(node->_.vector.table == NULL);
    }
    node->_.vector.len = newlen;
}

// consume value, but not vector
int vector_set(cell *node, index_t index, cell *value) {
    assert(cell_is_vector(node));
    if (index < 0 || index >= node->_.vector.len) {
        // out of bounds
        cell_unref(value);
        return 0;
    }
    cell_unref(node->_.vector.table[index]);
    node->_.vector.table[index] = value;
    return 1;
}

// ref value, leave vector
int vector_get(cell *node, index_t index, cell **valuep) {
    assert(cell_is_vector(node));
    if (index < 0 || index >= node->_.vector.len) {
        // out of bounds
        *valuep = NIL;
        return 0;
    }
    *valuep = cell_ref(node->_.vector.table[index]);
    return 1;
}

cell *cell_assoc() {
    cell *node = newcell(c_ASSOC);
    // no table required when empty
    return node;
}

cell *cell_special(const char *magic, void *ptr) {
    cell *node = newcell(c_SPECIAL);
    node->_.special.ptr = ptr;
    node->_.special.magic = magic;
    return node;
}

// TODO this will soon enough collapse
static void cell_free(cell *node) {
    assert(node && node->ref == 0);
    switch (node->type) {
    case c_LIST:
    case c_FUNC:
    case c_PAIR:
    case c_LAMBDA:
        cell_unref(node->_.cons.car);
        cell_unref(node->_.cons.cdr);
        free(node);
        break;
    case c_SYMBOL: 
        assert(oblist_teardown); // these exist only on oblist
        cell_unref(node->_.symbol.val);
        free(node->_.symbol.nam);
        free(node);
        break;
    case c_STRING:
        free(node->_.string.ptr);
        free(node);
        break;
    case c_INTEGER:
        free(node);
        break;
    case c_CFUNQ:
    case c_CFUN0:
    case c_CFUN1:
    case c_CFUN2:
    case c_CFUN3:
    case c_CFUNN:
        free(node);
        break;
    case c_VECTOR:
        vector_resize(node, 0);
        free(node);
        break;
    case c_ASSOC:
        assoc_drop(node);
        free(node);
        break;
    case c_SPECIAL:
        // TODO invoke magic free function
        free(node);
        break;
    }
}

// asym is allocated already
// return unreffed reference
cell *cell_oblist_item(char_t *asym) {
    cell *node = newcell(c_SYMBOL);
    // node->_.symbol.val = NIL; // TODO should probably be #void instead
    node->_.symbol.nam = asym; // allocated already
    return node;
}


// TODO inline
void cell_unref(cell *node) {
    if (node && --(node->ref) == 0) cell_free(node);
}

