/* TAB-P
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "opt.h"
#include "parse.h"
#include "oblist.h"
#include "cfun.h"
#include "io.h"

int main(int argc, char * const argv[]) {
    int opt;
    cell *ct;

    // opterr = 0;
    while ((opt = getopt(argc, argv, "OP")) >= 0) switch (opt) {
        case 'O':
            opt_showoblist = 1;
            break;
        case 'P':
            opt_showparse = 1;
            break;

        case '?': // error message generated // TODO can override with opterr
        case ':':
            break;
        default:
            assert(0);
    }

    cfun_init();

    while ((ct = expression())) {
        if (opt_showparse) {
            cell_print(stdout, ct);
        }
        printf(" --> ");
        ct = eval(ct, NULL);
        cell_print(stdout, ct);
        printf("\n");
        cell_unref(ct);
    }
    cfun_drop();
    oblist_drop(opt_showoblist);
    return 0;
}
