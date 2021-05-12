/* TAB P
 *
 * TODO should evaluation happen in functions? perhaps
 */

#include <assert.h>
#include <stdlib.h>

#include "cfun.h"
#include "cell.h"
#include "err.h"

// TODO check is this is right...
void apply_lambda(cell *fun, cell* args, environment **envp) {
    assert(fun->type == c_LAMBDA);
    cell *nam;
    cell *val;
    cell *argnames = cell_ref(fun->_.cons.car);
    cell *newassoc = cell_assoc();

    // pick up arguments one by one and add to assoc
    while (list_split(argnames, &nam, &argnames)) {
	assert(cell_is_symbol(nam));
	if (!list_split(args, &val, &args)) {
	    // TODO if more than one, have one message
	    cell_unref(error_rt1("missing value for", cell_ref(nam)));
	    val = cell_ref(hash_void);
	} else {
	    // TODO C recursion, should be avoided
	    val = eval(val, *envp);
	}
	if (!assoc_set(newassoc, nam, val)) {
	    cell_unref(val);
	    cell_unref(error_rt1("duplicate parameter name, value ignored", nam));
	}
    }
    if (args != NIL) {
	cell_unref(error_rt1("excess arguments ignored", args));
    }

    if (*envp && (*envp)->prog == NIL) {
	// end recursion, reuse environment
	cell_unref((*envp)->assoc);
	(*envp)->assoc = newassoc;
    } else {
	// add one level to environment
	struct env_s *newenv = malloc(sizeof(environment));
	newenv->prev = *envp;
	newenv->assoc = newassoc;
	*envp = newenv;
    }
    (*envp)->prog = cell_ref(fun->_.cons.cdr);
    cell_unref(fun);
}

// evalute the one thing in arg
cell *eval(cell *arg, environment *env) {
    environment *end_env = env;
    cell *end_prog = env ? env->prog : NIL; // unreffed
    cell *result = NIL;

    for (;;) {
	if (arg == NIL) {
	    result = NIL;
	} else switch (arg->type) {
	default:    // value is itself
	    result = arg;
	    break;

	case c_SYMBOL: // evaluate symbol
	    {
		environment *e = env;
		for (;;) {
		    if (!e) {
			// global
			result = cell_ref(arg->_.symbol.val);
			break;
		    }
		    if (assoc_get(e->assoc, arg, &result)) {
			break;
		    }
		    e = e->prev;
		}
	    }
	    cell_unref(arg);
	    break;

	case c_LIST:
	    {
		cell *fun;
		list_split(arg, &fun, &arg);
		fun = eval(fun, env);
		// TODO may consider moving evaluation out of functions

		switch (fun ? fun->type : c_LIST) {
		case c_CFUN:
		    {
			struct cell_s *(*def)(struct cell_s *, struct env_s *) = fun->_.cfun.def;
			cell_unref(fun);
			result = (*def)(arg, env);
		    }
		    break;

		case c_CFUN1:
		    {
			cell *(*def)(cell *) = fun->_.cfun1.def;
			cell_unref(fun);
			if (arg1(arg, &result)) { // if error, result is void
			    result = (*def)(eval(result, env));
			}
		    }
		    break;

		case c_CFUN2:
		    {
			cell *(*def)(cell *, cell *) = fun->_.cfun2.def;
			cell *b = NIL;
			cell_unref(fun);
			if (!arg2(arg, &result, &b)) { // if error, result is void
			    result = (*def)(eval(result, env), eval(b, env));
			}
		    }
		    break;

		case c_CFUNN:
		    {
			cell *(*def)(cell *) = fun->_.cfun1.def;
			cell_unref(fun);
			cell *e = NIL;
			cell **pp = &e;
			while (list_split(arg, &result, &arg)) {
			    *pp = cell_list(eval(result, env), NIL);
			    pp = &((*pp)->_.cons.cdr);
			}
			assert(arg == NIL);
			result = (*def)(e);
		    }
		    break;

		case c_LAMBDA:
		    apply_lambda(fun, arg, &env);
		    result = NIL; // TODO probably #void
		    break; // continue executing

		default: // not a function
		    // TODO show item before eval
		    cell_unref(arg);
		    result = error_rt1("not a function", fun);
		    break;
		}
	    }
	    break;

	case c_PAIR:
	    result = error_rt1("pair cannot be evaluated", arg);
	    break;
	}

	for (;;) {
	    if (!env) {
		return result; // reached top level
	    }
	    if (env == end_env && env->prog == end_prog) {
		// arg fully done
		return result;
	    }
	    if (env->prog == NIL) {
		// reached end of current level
		// drop one level of environment
		environment *prevenv = env->prev;
		cell_unref(env->assoc);
		free(env);
		env = prevenv;
	    } else {
		break;
	    }
	}
	// eval one more expression
	cell_unref(result);
	if (!list_split(env->prog, &arg, &(env->prog))) {
	    assert(0);
	}
    }
    assert(0);
    return NIL;
}
