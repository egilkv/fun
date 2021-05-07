/* TAB-P
 *
 */

#include <stdio.h>
#include <assert.h>

#include "parse.h"
#include "oblist.h"
#include "cfun.h"

int main() {
    cell *ct;

    cfun_init();

    while ((ct = expression())) {
        cell_print(ct);
        printf(" --> ");
        ct = cfun_eval(ct);
        cell_print(ct);
        printf("\n");
        cell_unref(ct);
    }
    oblist_drop();
    return 0;
}
