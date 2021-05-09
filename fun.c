/* TAB-P
 *
 */

#include <stdio.h>
#include <assert.h>

#include "parse.h"
#include "oblist.h"
#include "cfun.h"
#include "eval.h"

int main() {
    cell *ct;

    cfun_init();

    while ((ct = expression())) {
        cell_print(ct);
        printf(" --> ");
        ct = eval(ct, NIL);
        cell_print(ct);
        printf("\n");
        cell_unref(ct);
    }
    oblist_drop();
    return 0;
}
