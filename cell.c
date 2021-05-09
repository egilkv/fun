/* TAB-P
 *
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "oblist.h"

static  cell *newcell(celltype t) {
    cell *node = malloc(sizeof(cell));
    assert(node);
    memset(node, 0, sizeof(cell)); // TODO improve
    node->type = t;
    node->ref = 1;
    return node;
}

cell *cell_cons(cell *car, cell *cdr) {
    cell *node = newcell(c_CONS);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

// TODO inline
cell * cell_ref(cell *cp) {
    if (cp) ++(cp->ref);
    return cp;
}

// TODO inline
int cell_is_cons(cell *cp) {
    return cp && cp->type == c_CONS;
}

// TODO inline
int cell_is_vector(cell *cp) {
    return cp && cp->type == c_VECTOR;
}

// TODO in use?
cell *cell_car(cell *cp) {
    assert(cell_is_cons(cp));
    return cp->_.cons.car;
}

// TODO in use?
cell *cell_cdr(cell *cp) {
    assert(cell_is_cons(cp));
    return cp->_.cons.cdr;
}

// TODO inline
int cell_is_symbol(cell *cp) {
    return cp && cp->type == c_SYMBOL;
}

// TODO very dangerous
int cell_split(cell *cp, cell **carp, cell **cdrp) {
    if (cell_is_cons(cp)) {
        *carp = cp->_.cons.car;
	if (cdrp) *cdrp = cp->_.cons.cdr;
        else cell_unref(cp->_.cons.cdr);
        cp->_.cons.car = NIL;
        cp->_.cons.cdr = NIL;
        free(cp); // TODO garbage collection
        return 1;
     } else {
        *carp = NIL;
        *cdrp = NIL;
        return 0;
     }
}

cell *cell_symbol(char *symbol) {
    return oblist(symbol);
}

// symbol that is malloc'd already
cell *cell_asymbol(char *symbol) {
    return oblista(symbol);
}

cell *cell_astring(char *string) {
    cell *node = newcell(c_STRING);
    node->_.string.str = string;
    return node;
}

cell *cell_integer(integer_t integer) {
    cell *node = newcell(c_INTEGER);
    node->_.ivalue = integer;
    return node;
}

cell *cell_cfun(struct cell_s *(*fun)(struct cell_s *, struct cell_s *)) {
    cell *node = newcell(c_CFUN);
    node->_.cfun.def = fun;
    return node;
}

cell *cell_vector(index_t length) {
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

// TODO this will soon enough collapse
void cell_unref(cell *node) {
    if (node) {
        if (--(node->ref) == 0) switch (node->type) {
        case c_CONS:
        case c_LAMBDA:
            cell_unref(node->_.cons.car);
            cell_unref(node->_.cons.cdr);
            free(node);
            break;
        case c_SYMBOL:
            // cell is on oblist TODO special case, odd...
            break;
        case c_STRING:
            free(node->_.string.str);
            free(node);
            break;
        case c_INTEGER:
            free(node);
            break;
        case c_CFUN:
            free(node);
            break;
        case c_VECTOR:
            vector_resize(node, 0);
            free(node);
            break;
        case c_ASSOC:
            assert(0);
            break;
        }
    }
}
