/*  TAB-P
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "opt.h"
#include "parse.h"
#include "oblist.h"
#include "cfun.h"
#include "qfun.h"
#include "m_io.h"
#include "err.h"

int main(int argc, char * const argv[]) {
    int stop = -1;
    int opt;

    oblist_init();
    cfun_init();
    qfun_init();

    opterr = 0;
    while ((opt = getopt(argc, argv, "+:CGOPRS")) >= 0) switch (opt) {
        case 'C': // debug
            opt_showcode = 1;
            break;
        case 'G': // debug
            opt_showgc = 1;
            break;
        case 'O': // debug
            opt_showoblist = 1;
            break;
        case 'P': // debug
            opt_showparse = 1;
            break;
        case 'R':
            opt_noreadline = 1;
            break;
        case 'S':
            opt_assocsorted = 1;
            break;
        case '?':
            error_cmdopt("invalid option", optopt);
	    stop = 2;
	    break;
        case ':':
	    error_cmdopt("missing argument for option", optopt);
	    stop = 2;
	    break;
        default:
            assert(0);
    }
    if (stop >= 0) {
	exit(stop);
    }

    if (optind >= argc) { // filename on command line?
        // interactive mode
        lxfile infile;
        lxfile_init(&infile, stdin);
        infile.show_parse = (opt_showparse ? 1:0) | (opt_showcode ? 2:0);
        if (infile.is_terminal) {
            fprintf(stdout, "Have fun");
        }
        cfun_args(0, NULL);

        cell_unref(chomp_lx(&infile));
    } else {
        cfun_args(argc-optind, &argv[optind]); // argv[0] is program name
        // TODO consider setting linux program name too

        if (!chomp_file(argv[optind], NULL)) { // filename on command line
            error_cmdstr("cannot find file", argv[optind]);
        }
    }

    // always status 0 on normal exit
    return 0;
}
