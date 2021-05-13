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
#include "err.h"

static void chomp(FILE *f);

int stdin_is_terminal; // TODO move

int main(int argc, char * const argv[]) {
    int opt;
    int idx;

    cfun_init();
    stdin_is_terminal = isatty(fileno(stdin));

    // opterr = 0; TODO
    while ((opt = getopt(argc, argv, "OP")) >= 0) switch (opt) {
        case 'O':
            opt_showoblist = 1;
            break;
        case 'P':
            opt_showparse = 1;
            break;
        case '?': // error message generated // TODO can override with opterr
            error_cmdopt("unknown option", optopt);
	    break;
        case ':':
            error_cmdopt("bad option argument", optopt); // TODO really?
	    break;

        default:
            assert(0);
    }
    if (optind >= argc) { // no files on command line
        // interactive mode
        if (stdin_is_terminal) { // TODO more of this...
            fprintf(stdout, "Have fun");
        }
	chomp(stdin);
    } else {
        // filenames on command line?
        for (idx = optind; idx < argc; idx++) {
            FILE *f = fopen(argv[idx], "r");
            if (!f) {
                error_cmdstr("cannot find", argv[idx]);
            } else {
                chomp(f);
                fclose(f);
            }
	}
    }

    cfun_drop();
    oblist_drop(opt_showoblist);
    return 0;
}

static void chomp(FILE *f) {
    cell *ct;

    for (;;) {
        if (f == stdin && stdin_is_terminal) {
	    fprintf(stdout, "\n--> ");
	    fflush(stdout);
	}
	ct = expression(f);
	if (!ct) break; // eof
	if (f == stdin) {
	    if (opt_showparse) {
		cell_print(stdout, ct);
	    }
	}
        fprintf(stdout, " --> "); // TODO remove
        ct = eval(ct, NULL);
        if (f == stdin && stdin_is_terminal) {
            printf("\n");
	    cell_print(stdout, ct);

        } else if (f == stdin) { // TODO remove...
            // printf("\n");
	    cell_print(stdout, ct);
            printf("\n"); // TODO
	}
        cell_unref(ct);
    }
}
