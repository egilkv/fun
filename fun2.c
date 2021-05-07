/* TAB-P
 *
 */

#include <stdio.h>
#include <assert.h>

#include "parse.h"
#include "oblist.h"

int main() {

    cell *ct;
    while ((ct = expression())) {
        cell_print(ct);
        printf("\n");
        cell_drop(ct);
    }
    oblist_drop();
    return 0;
}
