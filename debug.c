//  TAB-P
//
//  functions to support debugging
//

#include <stdio.h>

#include "io.h"


// for use in debugger
void debug_prints(const char *msg) {
    fprintf(stdout, "%s", msg);
}

// for use in debugger
void debug_write(cell *ct) {
    cell_write(stdout, ct);
    fprintf(stdout, "\n");
    fflush(stdout);
}
