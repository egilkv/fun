//  TAB-P
//
//  functions to support debugging
//

#include <stdio.h>

#include "cfun.h"
#include "m_io.h"

// for use in debugger
void debug_prints(const char *msg) {
    fprintf(stdout, "%s", msg);
    fflush(stdout);
}

// for use in debugger
void debug_write(cell *ct) {
    cell_write(stdout, ct);
    fflush(stdout);
}

// for use in debugger
void debug_writeln(cell *ct) {
    cell_write(stdout, ct);
    fprintf(stdout, "\n");
    fflush(stdout);
}

// trace any value being passed
void debug_trace(cell *a) {
    debug_prints("\n*** "); // trace
    debug_write(a);
}

// enable trace for function
// TODO later, also for other things
int debug_traceon(cell *a) {
#if 0 // TODO
    if (a && a->type == c_FUNC) {
        // trace being used inline, presumably
        if ((cell_car(a) == hash_lambda)) {
            cell_unref(a->_.cons.car);
            a->_.cons.car = cell_ref(hash_lambdatrace); // silently replace with lambdatrace()
            return 1;
        }
        if ((cell_car(a) == hash_lambdatrace)) {
            return 1;
        }
        // TODO can we do other remarkable things here
        // TODO or perhaps do some other trace stuff
        return 0;
    }
#endif
    if (cell_is_symbol(a)) {
        a = a->_.symbol.val; // global value
    }
    if (a && a->type == c_CLOSURE) {
        a = a->_.cons.car;
    }
    // trace closures
    if (a) switch (a->type) {
    case c_CLOSURE0:
        a->type = c_CLOSURE0T; // turn on tracing for function
    case c_CLOSURE0T:
        return 1;
    default:
        break;
    }
    return 0;
}

// disable trace for function
int debug_traceoff(cell *a) {
    if (cell_is_symbol(a)) {
        a = a->_.symbol.val; // global value
    }
    if (a && a->type == c_CLOSURE) {
        a = a->_.cons.car;
    }
    // trace closures
    if (a) switch (a->type) {
    case c_CLOSURE0T:
        a->type = c_CLOSURE0; // turn off tracing for function
    case c_CLOSURE0:
        return 1;
    default:
        break;
    }
    return 0;
}

