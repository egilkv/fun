/*  TAB-P
 *
 *  these functions quote arguments, and are treated specially by the compiler
 */

#include <stdlib.h>

#include "cell.h"
#include "qfun.h"
#include "oblist.h"

static void qfun_exit(void);

void qfun_init() {
    // these values should be themselves
    hash_and      = symbol_self("#and");
    hash_apply    = symbol_self("#apply");
    hash_defq     = symbol_self("#defq");
    hash_if       = symbol_self("#if");
    hash_deflambda = symbol_self("#deflambda"); // TODO lambda dummy thingy for definitions
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
    oblist_set(hash_deflambda, NIL); // TODO lambda
    oblist_set(hash_or,        NIL);
    oblist_set(hash_quote,     NIL);
    oblist_set(hash_refq,      NIL);
}
