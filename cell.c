/* TAB-P
 *
 */

// #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cell.h"

static  cell *newcell(celltype t) {
    cell *node = malloc(sizeof(cell));
    assert(node);
    memset(node, 0, sizeof(cell));
    node->type = t;
    return node;
}

cell *cell_cons(cell *car, cell *cdr) {
    cell *node = newcell(c_CONS);
    node->car = car;
    node->cdr = cdr;
    return node;
}

cell *cell_symbol(char *symbol) {
    cell *node = newcell(c_SYMBOL);
    node->svalue = malloc(1+strlen(symbol));
    assert(node->svalue);
    strcpy(node->svalue, symbol);
    return node;
}

cell *cell_string(char *string) {
    cell *node = newcell(c_STRING);
    node->svalue = malloc(1+strlen(string));
    assert(node->svalue);
    strcpy(node->svalue, string);
    return node;
}

cell *cell_integer(long int integer) {
    cell *node = newcell(c_INTEGER);
    node->ivalue = integer;
    return node;
}

void cell_drop(cell *node) {
    if (node) switch (node->type) {
    case c_CONS:
        cell_drop(node->car);
        cell_drop(node->cdr);
        free(node);
        break;
    case c_STRING:
    case c_SYMBOL:
        free(node->svalue);
        free(node);
        break;
    case c_INTEGER:
        free(node);
        break;
    default:
        assert(0);
    }
}

