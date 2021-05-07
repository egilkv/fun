/* TAB-P
 *
 */

#include <stdio.h>
#include <assert.h>

#include "parse.h"
#include "oblist.h"

void show_cells(cell *ct);

static void show_list(cell *ct) {

    if (!ct) {
        // end of list
    } else if (ct->type == c_CONS) {
        show_cells(ct->_.cons.car); // TODO recursion?
        show_list(ct->_.cons.cdr);
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
        show_cells(ct->_.cons.car); // TODO recursion?
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
    oblist_drop();
    return 0;
}
