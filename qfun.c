/*  TAB-P
 *
 *  TODO should evaluation happen in functions? perhaps
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "cell.h"
#include "cmod.h"
#include "qfun.h"
#include "oblist.h"
#include "number.h"
#include "err.h"
#include "parse.h" // chomp_file
#include "debug.h"

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

static void qfun_exit(void);

void qfun_init() {
    symbol_set("#traceoff", cell_cfunN(cfunQ_traceoff)); // debugging TODO args must be quoted
    symbol_set("#traceon", cell_cfunN(cfunQ_traceon)); // debugging TODO args must be quoted

    // these values should be themselves
    hash_and      = symbol_self("#and");
    hash_apply    = symbol_self("#apply");
    hash_defq     = symbol_self("#defq");
    hash_if       = symbol_self("#if");
    hash_lambda   = symbol_self("#lambda");
    hash_or       = symbol_self("#or");
    hash_quote    = symbol_self("#quote");
    hash_refq     = symbol_self("#refq");

    atexit(qfun_exit);
}

static void qfun_exit(void) {
    // loose circular definitions
    oblist_set(hash_and,       NIL);
    oblist_set(hash_apply,     NIL);
    oblist_set(hash_defq,      NIL);
    oblist_set(hash_if,        NIL);
    oblist_set(hash_lambda,    NIL);
    oblist_set(hash_or,        NIL);
    oblist_set(hash_quote,     NIL);
    oblist_set(hash_refq,      NIL);
}
