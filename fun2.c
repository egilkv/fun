/* TAB-P
 *
 */

#include <stdio.h>
#include <assert.h>

#include "parse.h"

void show_cells(cell *ct);

static void show_list(cell *ct) {

    if (!ct) {
        // end of list
    } else if (ct->type == c_CONS) {
        show_cells(ct->car); // TODO recursion?
        show_list(ct->cdr);
    } else {
        printf(" . ");
        show_cells(ct);
    }
}

void show_cells(cell *ct) {

    if (!ct) {
        printf("{} ");

    } else switch (ct->type) {
    case c_CONS:
        printf("{ ");
        show_cells(ct->car); // TODO recursion?
        show_list(ct->cdr);
        printf("} ");
        break;
    case c_INTEGER:
        printf("%ld ", ct->ivalue);
        break;
    case c_STRING:
        printf("\"%s\" ",ct->svalue);
        break;
    case c_SYMBOL:
        printf("%s ", ct->svalue);
        break;
    default:
        assert(0);
    }
}

int main() {

    cell *ct;
    while ((ct = expression())) {
        show_cells(ct);
        printf("\n");
        cell_drop(ct);
    }
    return 0;
}
