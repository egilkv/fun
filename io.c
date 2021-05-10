/* TAB-P
 *
 */

#include <stdio.h>
#include <assert.h>

#include "cell.h"
#include "io.h"

static void show_list(FILE *out, cell *ct) {
    if (!ct) {
        // end of list
    } else if (cell_is_cons(ct)) {
        cell_print(out, cell_car(ct)); // TODO recursion?
        if (cell_cdr(ct)) {
            if (cell_is_cons(cell_cdr(ct))) {
                fprintf(out, ", ");
                show_list(out, cell_cdr(ct));
            } else {
                fprintf(out, " . "); // TODO not supported on read
                cell_print(out, ct);
            }
        }
    } else {
        assert(0);
    }
}

void cell_print(FILE *out, cell *ct) {

    if (!ct) {
        fprintf(out, "#()");
    } else switch (ct->type) {
    case c_CONS:
        fprintf(out, "#(");
        // cell_print(out, ct->_.cons.car);
        // show_list(out, ct->_.cons.cdr);
        show_list(out, ct);
        fprintf(out, ")");
        break;
    case c_INTEGER:
        fprintf(out, "%ld", ct->_.ivalue);
        break;
    case c_STRING:
        fprintf(out, "\"%s\"",ct->_.string.str);
        break;
    case c_SYMBOL:
        fprintf(out, "%s", ct->_.symbol.nam);
        break;
    case c_LAMBDA:
        fprintf(out, "#lambda"); // TODO something better
        break;
    case c_CFUN:
        fprintf(out, "#cdef"); // TODO something better
        break;
    case c_VECTOR:
        fprintf(out, "#vector[%ld]", ct->_.vector.len); // TODO something better
        break;
    case c_ASSOC:
        fprintf(out, "#assoc[]"); // TODO something better
        break;
    }
}
