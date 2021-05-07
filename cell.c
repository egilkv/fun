/* TAB-P
 *
 */

// #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cell.h"
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

// TODO: should change to oblist-based thingy
cell *cell_symbol(char *symbol) {
    cell *node = newcell(c_SYMBOL);
    char *sym = malloc(1+strlen(symbol));
    assert(sym);
    strcpy(sym, symbol);
    node->_.symbol.nam = oblist(sym);
    return node;
}

// symbol that is malloc'd already
cell *cell_asymbol(char *symbol) {
    cell *node = newcell(c_SYMBOL);
    node->_.symbol.nam = oblist(symbol);
    return node;
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

void cell_drop(cell *node) {
    if (node) switch (node->type) {
    case c_CONS:
        cell_drop(node->_.cons.car);
        cell_drop(node->_.cons.cdr);
        free(node);
        break;
    case c_SYMBOL:
        // _.symbol.nam is owned by oblist
        free(node);
        break;
    case c_STRING:
        free(node->_.string.str);
        free(node);
        break;
    case c_INTEGER:
        free(node);
        break;
    default:
        assert(0);
    }
}

