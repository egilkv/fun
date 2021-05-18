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

static void chomp(lxfile *f);

int main(int argc, char * const argv[]) {
    int opt;
    lxfile infile;

    cfun_init();

    // opterr = 0; TODO
    while ((opt = getopt(argc, argv, "+OP")) >= 0) switch (opt) {
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
    if (optind >= argc) { // filename on command line?
        // interactive mode
        lxfile_init(&infile, stdin);
        if (infile.is_terminal) {
            fprintf(stdout, "Have fun");
        }
        cfun_args(0, NULL);
        chomp(&infile);
    } else {
	// filename on command line
	FILE *f = fopen(argv[optind], "r");
	if (!f) {
	    error_cmdstr("cannot find", argv[optind]);
	} else {
	    lxfile_init(&infile, f);
            // TODO consider setting linux program name too
            cfun_args(argc-optind, &argv[optind]); // argv[0] is program name
	    chomp(&infile);
	    fclose(infile.f);
	}
    }

    cfun_drop();
    oblist_drop(opt_showoblist);
    return 0;
}

static void chomp(lxfile *lxf) {
    cell *ct;

    for (;;) {
        ct = expression(lxf);
	if (!ct) break; // eof
        if (lxf->f == stdin) {
	    if (opt_showparse) {
		cell_print(stdout, ct);
                printf(" ==> ");
	    }
	}
        ct = eval(ct, NULL);
        if (lxf->f == stdin) {
            cell_print(stdout, ct);
            if (!(lxf->is_terminal)) {
                printf("\n");
            }
	}
        cell_unref(ct);
    }
    if (lxf->is_terminal) {
        printf("\n");
    }
}
