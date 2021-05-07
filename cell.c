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
    memset(node, 0, sizeof(cell));
    node->type = t;
    return node;
}

cell *cell_cons(cell *car, cell *cdr) {
    cell *node = newcell(c_CONS);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

// TODO inline
int cell_is_cons(cell *cp) {
    return cp && cp->type == c_CONS;
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

cell *cell_integer(long int integer) {
    cell *node = newcell(c_INTEGER);
    node->_.ivalue = integer;
    return node;
}

cell *cell_cfun(struct cell_s *(*fun)(struct cell_s *)) {
    cell *node = newcell(c_CFUN);
    node->_.cfun.def = fun;
    return node;
}

// TODO this will soon enough collapse
void cell_drop(cell *node) {
    if (node) switch (node->type) {
    case c_CONS:
        cell_drop(node->_.cons.car);
        cell_drop(node->_.cons.cdr);
        free(node);
        break;
    case c_SYMBOL:
        // cell is on oblist
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
    default:
        assert(0);
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
    case c_CFUN:
        printf("#fundef "); // TODO something better
        break;
    default:
        assert(0);
    }
}
