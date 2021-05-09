/* TAB-P
 *
 */

#include <stdio.h>
#include <assert.h>

#include "parse.h"
#include "oblist.h"
#include "cfun.h"
#include "eval.h"
#include "io.h"

int main() {
    cell *ct;

    cfun_init();

    while ((ct = expression())) {
        cell_print(stdout, ct);
        printf(" --> ");
        ct = eval(ct, NIL);
        cell_print(stdout, ct);
        printf("\n");
        cell_unref(ct);
    }
    oblist_drop();
    return 0;
}
