//  TAB-P
//
//  functions to support debugging
//

#include <stdio.h>

#include "cfun.h"
#include "oblist.h"
#include "number.h"
#include "parse.h"
#include "m_io.h"
#include "run.h"
#include "err.h"

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

// debugging, trace value being passed
static cell *cfun1_trace(cell *a) {
    debug_trace(a);
    return a;
}

// debugging, run garbage collection
static cell *cfunN_gc(cell *args) {
    integer_t nodes;
    arg0(args);
    nodes = oblist_sweep();
    return cell_integer(nodes);
}

// debugging, breakpoint
static cell *cfunN_bp(cell *args) {
    cell *run_env = current_run_env();
    cell *env0 = NIL;
    arg0(args);
    // TODO check if terminal is connected
    // TODO 1st argument is returned here...

    if (run_env == NIL) { // breakpoint in a function?
        // prevenv and prog are both NIL
        env0 = cell_env(NIL, NIL, cell_ref(env_assoc(run_env)), cell_ref(env_cont_env(run_env)));
    }
    interactive_mode("*breakpoint*", "\nbp> ", env0);
    return cell_void();
}

// debugging, enable trace, return first (valid) argument
static cell *cfunQ_traceon(cell *args) {
    cell *result = cell_void();
    cell *arg;
    while (list_pop(&args, &arg)) {
        if (!debug_traceon(arg)) {
            arg = error_rt1("cannot enable trace for", arg);
        }
        if (result == hash_void) {
            cell_unref(result);
            result = arg;
        } else {
            cell_unref(arg);
        }
    }
    return result;
}

// debugging, disable trace, return first (valid) argument
static cell *cfunQ_traceoff(cell *args) {
    cell *result = cell_void();
    cell *arg;
    while (list_pop(&args, &arg)) {
        if (!debug_traceoff(arg)) {
            arg = error_rt1("cannot disable trace for", arg);
        }
        if (result == hash_void) {
            cell_unref(result);
            result = arg;
        } else {
            cell_unref(arg);
        }
    }
    return result;
}

void debug_init() {
    symbol_set("#gc",       cell_cfunN(cfunN_gc)); // debugging
    symbol_set("#trace",    cell_cfun1(cfun1_trace)); // debugging
    symbol_set("#bp",       cell_cfunN(cfunN_bp)); // debugging
    symbol_set("#traceoff", cell_cfunN(cfunQ_traceoff)); // debugging TODO args must be quoted
    symbol_set("#traceon",  cell_cfunN(cfunQ_traceon)); // debugging TODO args must be quoted

}
