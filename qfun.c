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
#include "eval.h"
#include "err.h"
#include "parse.h" // chomp_file
#include "debug.h"

#if !HAVE_COMPILER

static cell *cfunQ_defq(cell *args, cell **envp) {
    cell *a, *b;
    if (!arg2(args, &a, &b)) {
        return cell_ref(cell_void()); // error
    }
    b = eval(b, envp);
    return defq(a, b, envp);
}

static cell *cfunQ_apply(cell *args, cell **envp) {
    cell *func;
    cell *collectargs = NIL;
    cell **nextp = &collectargs;
    cell *a;

    if (!list_pop(&args, &func)) {
	cell_unref(args);
        return error_rt0("missing function");
    }
    func = eval(func, envp);
    while (list_pop(&args, &a)) {
        a = eval(a, envp);
        if (!args) {
            // last argument is special
            *nextp = a;
            break;
        }
        // collect arguments on a list
        *nextp = cell_list(a, NIL);
        nextp = &((*nextp)->_.cons.cdr);
    }
    // TODO can be made more efficient
    return eval(cell_func(func, collectargs), envp);
}

static cell *cfunQ_and(cell *args, cell **envp) {
    int bool = 1;
    cell *a;

    while (list_pop(&args, &a)) {
        a = eval(a, envp);
	if (!get_boolean(a, &bool, args)) {
            return cell_void(); // error
	}
	if (!bool) {
	    cell_unref(args);
	    break;
	}
    }
    return cell_ref(bool ? hash_t : hash_f);
}

static cell *cfunQ_or(cell *args, cell **envp) {
    int bool = 0;
    cell *a;

    while (list_pop(&args, &a)) {
        a = eval(a, envp);
	if (!get_boolean(a, &bool, args)) {
            return cell_void(); // error
	}
	if (bool) {
	    cell_unref(args);
	    break;
	}
    }
    return cell_ref(bool ? hash_t : hash_f);
}

static cell *cfunQ_if(cell *args, cell **envp) {
    int bool;
    cell *a, *b, *c;

    if (!list_pop(&args, &a)) {
	cell_unref(args);
	return error_rt0("missing condition for if");
    }
    if (!list_pop(&args, &b)) {
	cell_unref(a);
	cell_unref(args);
	return error_rt0("missing value if true for if");
    }
    if (list_pop(&args, &c)) { // if-then-else
	arg0(args);
    } else {
	c = cell_void(); // TODO can optimize
    }
    a = eval(a, envp);
    if (!get_boolean(a, &bool, args)) {
        return cell_void(); // error
    }
    if (bool) {
	cell_unref(c);
	a = b;
    } else {
	cell_unref(b);
	a = c;
    }
#if 1 // TODO enable...
    if (*envp) {
	// evaluate in-line
        *env_progp(*envp) = cell_list(a, env_prog(*envp));
        return NIL;
    } else 
#endif
    {
        return eval(a, envp);
    }
}

static cell *cfunQ_quote(cell *args, cell **envp) {
    cell *a;
    arg1(args, &a); // sets void if error
    return a;
}

static cell *cfunQ_lambda(cell *args, cell **envp) {
    cell *paramlist;
    cell *cp;
    if (!list_pop(&args, &paramlist)) {
        return error_rt1("missing function parameter list", args);
    }
    // TODO consider moving to parser
    // check for proper and for duplicate parameters
    cp = paramlist;
    while (cp) {
        cell *param;
        assert(cell_is_list(cp));
        param = cell_car(cp);
        if (cell_is_label(param)) {
            param = param->_.cons.car;
            // TODO evaluate default value
        }
        if (param == hash_ellip) {
            // TODO what to do about this?
            if (cell_cdr(cp)) {
                cell_unref(error_rt1("ellipsis must be last parameter", cp));
                cp = NIL;
            }
        } else if (!cell_is_symbol(param)) {
            // TODO what to do about this?
            cell_unref(error_rt1("parameter not a symbol", cell_ref(param)));
        } else if (exists_on_list(cell_cdr(cp), param)) {
            // TODO what to do about this?
            cell_unref(error_rt1("duplicate parameter name", cell_ref(param)));
        }
        cp = cell_cdr(cp);
    }

    // all functions are continuations
    cp = cell_lambda(paramlist, args);
    if (*envp != NIL) {
        cp = cell_closure(cp, cell_ref(*envp));
    }
    return cp;
}

static cell *cfunQ_refq(cell *args, cell **envp) {
    cell *a, *b;
    if (arg2(args, &a, &b)) {
        a = cfun2_ref(eval(a, envp), b);
    }
    return a;
}
#endif // !HAVE_COMPILER

// debugging, enable trace, return first (valid) argument
static cell *cfunQ_traceon(cell *args, cell **envp) {
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
static cell *cfunQ_traceoff(cell *args, cell **envp) {
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

#if !HAVE_COMPILER
void qfun_init() {
    // TODO hash_and etc are unrefferenced, and depends on oblist
    //      to keep symbols in play
    hash_and      = oblistv("#and",      cell_cfunQ(cfunQ_and));
                    oblistv("#apply",    cell_cfunQ(cfunQ_apply));
    hash_defq     = oblistv("#defq",     cell_cfunQ(cfunQ_defq));
    hash_if       = oblistv("#if",       cell_cfunQ(cfunQ_if));
    hash_lambda   = oblistv("#lambda",   cell_cfunQ(cfunQ_lambda));
    hash_or       = oblistv("#or",       cell_cfunQ(cfunQ_or));
    hash_quote    = oblistv("#quote",    cell_cfunQ(cfunQ_quote));
    hash_refq     = oblistv("#refq",     cell_cfunQ(cfunQ_refq));

                    oblistv("#traceoff", cell_cfunQ(cfunQ_traceoff)); // debugging
                    oblistv("#traceon",  cell_cfunQ(cfunQ_traceon)); // debugging
}

#else // !HAVE_COMPILER

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

                    oblistv("#traceoff", cell_cfunQ(cfunQ_traceoff)); // debugging
                    oblistv("#traceon",  cell_cfunQ(cfunQ_traceon)); // debugging

    // values should be themselves
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
#endif
