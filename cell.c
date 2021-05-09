/* TAB-P
 *
 */

#include <stdio.h> // for cell_print()
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

int cell_split(cell *cp, cell **carp, cell **cdrp) {
    if (cell_is_cons(cp)) {
        *carp = cp->_.cons.car;
        *cdrp = cp->_.cons.cdr;
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

cell *cell_vector(arraylen_t length) {
    cell *node = newcell(c_VECTOR);
    node->_.vector.len = length;
    if (length > 0) { // if length==0 table is NULL
        // TODO have some sanity check on vector length
        node->_.vector.table = malloc(length * sizeof(cell *));
        assert(node->_.vector.table);
        memset(node->_.vector.table, 0, length * sizeof(cell *));
    }
}

void vector_resize(cell* node, arraylen_t newlen) {
    // TODO have some sanity check on new vector length
    arraylen_t oldlen;
    assert(cell_is_vector(node));
    oldlen = node->_.vector.len;

    if (oldlen > 0 && oldlen > newlen) {
	arraylen_t i;
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
int vector_set(cell *node, arraylen_t index, cell *value) {
    assert(cell_is_vector(node));
    if (index < 0 || index >= node->_.vector.len) {
        // out of bounds
        cell_unref(value);
        return 0;
    }
    cell_unref(node->_.vector.table[index]);
    node->_.vector.table[index] = value;
}

// ref value, leave vector
int vector_get(cell *node, arraylen_t index, cell **valuep) {
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
        default:
            assert(0);
        }
    }
}

static void show_list(cell *ct) {
    if (!ct) {
        // end of list
    } else if (ct->type == c_CONS) {
        cell_print(ct->_.cons.car); // TODO recursion?
        show_list(ct->_.cons.cdr);
    } else {
        printf(" . ");
        cell_print(ct);
    }
}

void cell_print(cell *ct) {

    if (!ct) {
        printf("{} ");
    } else switch (ct->type) {
    case c_CONS:
        printf("{ ");
	cell_print(ct->_.cons.car); // TODO recursion?
        show_list(ct->_.cons.cdr);
        printf("} ");
        break;
    case c_INTEGER:
        printf("%ld ", ct->_.ivalue);
        break;
    case c_STRING:
        printf("\"%s\" ",ct->_.string.str);
        break;
    case c_SYMBOL:
        printf("%s ", ct->_.symbol.nam);
        break;
    case c_LAMBDA:
        printf("#lambda "); // TODO something better
        break;
    case c_CFUN:
        printf("#cdef "); // TODO something better
        break;
    case c_VECTOR:
        printf("#vector[%ld] ", ct->_.vector.len); // TODO something better
        break;
    default:
        assert(0);
    }
}
