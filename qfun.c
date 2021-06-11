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
    cell *result = cell_ref(hash_void);
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
    cell *result = cell_ref(hash_void);
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
    // these are mere placeholders
    hash_and      = oblistv("#and",      NIL);
    hash_apply    = oblistv("#apply",    NIL);
    hash_defq     = oblistv("#defq",     NIL);
    hash_if       = oblistv("#if",       NIL);
    hash_lambda   = oblistv("#lambda",   NIL);
    hash_or       = oblistv("#or",       NIL);
    hash_quote    = oblistv("#quote",    NIL);
    hash_refq     = oblistv("#refq",     NIL);
    hash_prev     = oblistv("#previous", NIL);

                    oblistv("#traceoff", cell_cfunN(cfunQ_traceoff)); // debugging TODO args must be quoted
                    oblistv("#traceon",  cell_cfunN(cfunQ_traceon)); // debugging  TODO args must be quoted

    // these values should be themselves
    oblist_set(hash_and,       cell_ref(hash_and));
    oblist_set(hash_apply,     cell_ref(hash_apply));
    oblist_set(hash_defq,      cell_ref(hash_defq));
    oblist_set(hash_if,        cell_ref(hash_if));
    oblist_set(hash_lambda,    cell_ref(hash_lambda));
    oblist_set(hash_or,        cell_ref(hash_or));
    oblist_set(hash_quote,     cell_ref(hash_quote));
    oblist_set(hash_refq,      cell_ref(hash_refq));

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
