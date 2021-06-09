/*  TAB-P
 *
 *  eval and helpers
 *  TODO should evaluation happen in functions? perhaps
 */

#include <assert.h>
#include <stdlib.h>

#include "cmod.h"
#include "eval.h"
#include "oblist.h"
#include "err.h"
#include "debug.h"

#if !HAVE_COMPILER

static void insert_prog(cell **envp, cell *newprog, cell* newassoc, cell *contenv) {
#if 0 // TODO does not work since we may still have stuff up our C stack that needs the assoc
    if (*envp && env_prog(*envp) == NIL) {
        // tail call, we can drop current environment
        cell *newenv = cell_env(cell_ref(env_prev(*envp)), newprog, newassoc, contenv);
        cell_unref(*envp);
	*envp = newenv;
    } else
#endif
    {
	// add one level to environment
        cell *newenv = cell_env(*envp, newprog, newassoc, contenv);
	*envp = newenv;
    }
}

// deal with variadic function argument
static cell *pick_variadic(cell *val1, cell **argsp, cell **envp) {
    cell *result = NIL;
    cell **nextp = &result;
    do {
        val1 = eval(val1, envp);
        *nextp = cell_list(val1, NIL);
        nextp = &((*nextp)->_.cons.cdr);
    } while (list_pop(argsp, &val1));
    return result;
}

// assume fun, args and contenv are all reffed
static void apply_closure(cell *fun, cell* args, cell *contenv, cell **envp) {
    int gotlabel = 0;
    cell *nam;
    cell *val;
    assert(fun && (fun->type == c_CLOSURE0 || fun->type == c_CLOSURE0T));
    cell *params = cell_ref(fun->_.cons.car);
    cell *newassoc = cell_assoc();

    if (fun->type == c_CLOSURE0T) {
        debug_prints("\n*** "); // trace
    }

    // pick up actual arguments one by one
    while (list_pop(&args, &val)) {

        if (cell_is_list(params) && cell_car(params) == hash_ellip) {
	    // special case for ellipsis, evaluate rest and place on list
            val = pick_variadic(val, &args, envp);
            cell_unref(params);
	    params = NIL;
            nam = cell_ref(hash_ellip);

	} else if (cell_is_label(val)) {
            ++gotlabel;
            label_split(val, &nam, &val);
            if (!cell_is_symbol(nam)) {
                cell_unref(error_rt1("parameter label must be a symbol", nam));
                cell_unref(val);
                continue;
            }
	    // TODO disallow ellipsis here
            // check if label actually exists on parameter list
            // TODO takes much time
            if (!exists_on_list(params, nam)) {
                cell_unref(error_rt1("no such parameter label", nam));
                cell_unref(val);
                continue;
            }
	    // TODO C recursion, should be avoided
	    val = eval(val, envp);

        } else if (gotlabel) {
            if (exists_on_list(params, hash_ellip)) {
		// variadic function mixed with labels
                val = pick_variadic(val, &args, envp);
                cell_unref(params);
                params = NIL;
                nam = cell_ref(hash_ellip);
            } else {
                // TODO this can be implemented, but should it?
                cell_unref(error_rt1("ignore unlabelled argument following labelled", val));
                continue;
            }
        } else {
            // match unlabeled argument to parameter list
            if (!list_pop(&params, &nam)) {
                arg0(cell_list(val, args)); // too many args?
                break;
            }
            if (cell_is_label(nam)) { // a default value is supplied?
                label_split(nam, &nam, NILP);
            }
	    assert(cell_is_symbol(nam));

	    // TODO C recursion, should be avoided
	    val = eval(val, envp);
        }

        if (fun->type == c_CLOSURE0T) {
            debug_write(nam);
            debug_prints(": ");
            debug_write(val);
        }
	if (!assoc_set(newassoc, nam, val)) {
            // TODO checked earlier, so should not happen
	    cell_unref(val);
	    cell_unref(error_rt1("duplicate parameter name, value ignored", nam));
	}
    }

    // check for any args with no value supplied for?
    while (list_pop(&params, &nam)) {
        // TODO if more than one, have one message
        if (cell_is_label(nam)) { // a default value is supplied?
            label_split(nam, &nam, &val);
            if (assoc_get(newassoc, nam, NILP)) {
                cell_unref(nam);
                cell_unref(val);
            } else {
                // use default value
                // TODO eval when?
                if (!assoc_set(newassoc, nam, val)) {
                    assert(0);
                }
            }
        } else { // no default
            if (assoc_get(newassoc, nam, NILP)) {
                cell_unref(nam);
            } else if (nam == hash_ellip) {
                if (!assoc_set(newassoc, nam, NIL)) { // empty list
                    assert(0);
                }
            } else {
                cell_unref(error_rt1("missing value for", cell_ref(nam)));
                if (!assoc_set(newassoc, nam, cell_void())) { // error
                    assert(0);
                }
            }
        }
    }

    if (fun->type == c_CLOSURE0T) {
        // debug_prints("\n");
    }
    insert_prog(envp, cell_ref(fun->_.cons.cdr), newassoc, contenv);
    cell_unref(fun);
}

// evalute the one thing in arg
cell *eval(cell *arg, cell **envp) {
    cell *end_env = *envp;
    cell *end_prog = *envp ? env_prog(*envp) : NIL; // unreffed
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
                cell *e = *envp;
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

                fun = eval(fun, envp);
		// TODO may consider moving evaluation out of functions

		switch (fun ? fun->type : c_LIST) {
		case c_CFUNQ:
		    {
                        struct cell_s *(*def)(cell *, cell **) = fun->_.cfunq.def;
			cell_unref(fun);
                        result = (*def)(args, envp);
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
                            result = (*def)(eval(result, envp));
			}
		    }
		    break;

		case c_CFUN2:
		    {
			cell *(*def)(cell *, cell *) = fun->_.cfun2.def;
			cell *b = NIL;
			cell_unref(fun);
                        if (arg2(args, &result, &b)) { // if error, result is void
                            result = (*def)(eval(result, envp), eval(b, envp));
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
                            result = (*def)(eval(result, envp), eval(b, envp), eval(c, envp));
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
                            *pp = cell_list(eval(result, envp), NIL);
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

                        apply_closure(lambda, args, contenv, envp);
                    }
		    result = NIL; // TODO probably #void
		    break; // continue executing

                case c_CLOSURE0:
                case c_CLOSURE0T:
                    apply_closure(fun, args, NIL, envp);
		    result = NIL; // TODO probably #void
		    break; // continue executing

                case c_ASSOC:
                case c_VECTOR:
                case c_LIST:
                case c_STRING:
                    if (arg1(args, &result)) { // if error, result is void
                        result = cfun2_ref(fun, eval(result, envp));
                    }
		    break;

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
                range_split(arg, &b1, &b2);
                if (b1) {
                    b1 = eval(b1, envp);
                }
                if (b2) {
                    b2 = eval(b2, envp);
                }
                result = cell_range(b1, b2);
            }
	    break;

        case c_LABEL: // evaluate right part only
            {
                cell *b1, *b2;
                label_split(arg, &b1, &b2);
		if (b1) switch (b1->type) {
		case c_STRING:
		    // TODO consider if we should sanity check this
		    // TODO and what about NULs in strings
		    if (b1->_.string.len > 0) {
			cell *b1a = cell_symbol(b1->_.string.ptr);
			cell_unref(b1);
			b1 = b1a;
		    }
		    break;
		case c_SYMBOL:
		    // a symbol stays as is
		    break;
		default:
                    b1 = eval(b1, envp); // evaluate in all other cases
		    break;
                }
                if (b2) {
                    b2 = eval(b2, envp);
                }
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
            if (!*envp) {
		return result; // reached top level
	    }
            if (*envp == end_env && env_prog(*envp) == end_prog) {
                // arg fully done // TODO this is a hack, really...
                return result;
	    }
            if (env_prog(*envp) == NIL) {
		// reached end of current level
		// drop one level of environment
                cell *prevenv = cell_ref(env_prev(*envp));
                cell_unref(*envp);
                *envp = prevenv;
	    } else {
		break;
	    }
	}
	// eval one more expression
	cell_unref(result);
        // TODO make more efficient
        if (!list_pop(env_progp(*envp), &arg)) {
            // assert(0);
            return error_rt1("bad program", env_prog(*envp));
        }
    }
    assert(0);
    return NIL;
}
#endif // !HAVE_COMPILER
