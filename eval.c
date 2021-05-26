/*  TAB-P
 *
 *  TODO should evaluation happen in functions? perhaps
 */

#include <assert.h>
#include <stdlib.h>

#include "cmod.h"
#include "err.h"

static void insert_prog(cell **envp, cell *newprog, cell* newassoc, cell *contenv) {
#if 0 // TODO enable end recursion detection BUG: does not work
    if (*envp && env_prog(*envp) == NIL) {
	// end recursion, reuse environment
        env_replace(*envp, newprog, newassoc, contenv);
    } else 
#endif
    {
	// add one level to environment
        cell *newenv = cell_env(*envp, newprog, newassoc, contenv);
	*envp = newenv;
    }
}

// assume fun, args and contenv are all reffed
static void apply_closure(cell *fun, cell* args, cell *contenv, cell **envp) {
    cell *nam;
    cell *val;
    assert(fun->type == c_CLOSURE0);
    cell *argnames = cell_ref(fun->_.cons.car);
    cell *newassoc = cell_assoc();

    // pick up arguments one by one and add to assoc
    while (list_pop(&argnames, &nam)) {
	assert(cell_is_symbol(nam));
        if (!list_pop(&args, &val)) {
	    // TODO if more than one, have one message
	    cell_unref(error_rt1("missing value for", cell_ref(nam)));
            val = cell_void();
	} else {
	    // TODO C recursion, should be avoided
	    val = eval(val, *envp);
	}
	if (!assoc_set(newassoc, nam, val)) {
	    cell_unref(val);
	    cell_unref(error_rt1("duplicate parameter name, value ignored", nam));
	}
    }
    arg0(args); // too many args?
    insert_prog(envp, cell_ref(fun->_.cons.cdr), newassoc, contenv);
    cell_unref(fun);
}

// evalute the one thing in arg
cell *eval(cell *arg, cell *env) {
    cell *end_env = env;
    cell *end_prog = env ? env_prog(env) : NIL; // unreffed
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
                cell *e = env;
		for (;;) {
		    if (!e) {
			// global
			result = cell_ref(arg->_.symbol.val);
			break;
		    }
                    if (assoc_get(env_assoc(e), arg, &result)) {
			break;
		    }
                    e = env_cont_env(e);
		}
	    }
	    cell_unref(arg);
	    break;

	case c_FUNC:
	    {
                cell *fun = cell_ref(arg->_.cons.car);
                cell *args = cell_ref(arg->_.cons.cdr);
                cell_unref(arg);
                arg = NIL;

		fun = eval(fun, env);
		// TODO may consider moving evaluation out of functions

		switch (fun ? fun->type : c_LIST) {
		case c_CFUNQ:
		    {
                        struct cell_s *(*def)(cell *, cell *) = fun->_.cfunq.def;
			cell_unref(fun);
                        result = (*def)(args, env);
		    }
		    break;

		case c_CFUN0:
		    {
			cell *(*def)(void) = fun->_.cfun0.def;
			cell_unref(fun);
                        arg0(args);
			result = (*def)();
		    }
		    break;

		case c_CFUN1:
		    {
			cell *(*def)(cell *) = fun->_.cfun1.def;
			cell_unref(fun);
                        if (arg1(args, &result)) { // if error, result is void
			    result = (*def)(eval(result, env));
			}
		    }
		    break;

		case c_CFUN2:
		    {
			cell *(*def)(cell *, cell *) = fun->_.cfun2.def;
			cell *b = NIL;
			cell_unref(fun);
                        if (arg2(args, &result, &b)) { // if error, result is void
			    result = (*def)(eval(result, env), eval(b, env));
			}
		    }
		    break;

		case c_CFUN3:
		    {
			cell *(*def)(cell *, cell *, cell *) = fun->_.cfun3.def;
			cell *b = NIL;
			cell *c = NIL;
			cell_unref(fun);
                        if (arg3(args, &result, &b, &c)) { // if error, result is void
			    result = (*def)(eval(result, env), eval(b, env), eval(c, env));
			}
		    }
		    break;

		case c_CFUNN:
		    {
			cell *(*def)(cell *) = fun->_.cfun1.def;
			cell_unref(fun);
			cell *e = NIL;
			cell **pp = &e;
                        while (list_pop(&args, &result)) {
			    *pp = cell_list(eval(result, env), NIL);
			    pp = &((*pp)->_.cons.cdr);
			}
                        assert(args == NIL);
			result = (*def)(e);
		    }
		    break;

                case c_CLOSURE:
                    // a continuation, i.e. a function being referenced
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        assert(lambda && lambda->type == c_CLOSURE0);

                        apply_closure(lambda, args, contenv, &env);
                    }
		    result = NIL; // TODO probably #void
		    break; // continue executing

                case c_CLOSURE0:
                    apply_closure(fun, args, NIL, &env);
		    result = NIL; // TODO probably #void
		    break; // continue executing

		default: // not a function
		    // TODO show item before eval
                    cell_unref(args);
		    result = error_rt1("not a function", fun);
		    break;
		}
	    }
	    break;

        case c_RANGE: // evaluate lower and upper bound
            {
                cell *b1, *b2;
                pair_split(arg, &b1, &b2);
                if (b1) b1 = eval(b1, env);
                if (b2) b2 = eval(b2, env);
                result = cell_range(b1, b2);
            }
	    break;

        case c_LABEL: // evaluate right part only
            {
                cell *b1, *b2;
                pair_split(arg, &b1, &b2);
                if (b2) b2 = eval(b2, env);
                result = cell_label(b1, b2);
            }
	    break;

        case c_LIST:
	    result = error_rt1("list cannot be evaluated", arg);
	    break;
	case c_PAIR:
	    result = error_rt1("pair cannot be evaluated", arg);
	    break;
	}

        // TODO can all be made more efficient
	for (;;) {
	    if (!env) {
		return result; // reached top level
	    }
            if (env == end_env && env_prog(env) == end_prog) {
                // arg fully done // TODO this is a hack, really...
                return result;
	    }
            if (env_prog(env) == NIL) {
		// reached end of current level
		// drop one level of environment
                cell *prevenv = cell_ref(env_prev(env));
                cell_unref(env);
		env = prevenv;
	    } else {
		break;
	    }
	}
	// eval one more expression
	cell_unref(result);
        // TODO make more efficient
        if (!list_pop(env_progp(env), &arg)) {
            // assert(0);
            return error_rt1("bad program", env_prog(env));
        }
    }
    assert(0);
    return NIL;
}
