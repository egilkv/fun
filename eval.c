/* TAB P
 *
 * TODO should evaluation happen in functions? perhaps
 */

#include <assert.h>

#include "cfun.h"
#include "eval.h"
#include "err.h"

// if a is NIL, return value otherwise complain and return void
cell *verify_nil(cell *a, cell *value) {
    if (a) {
	cell_unref(error_rt1("improper list", a));
	cell_unref(value);
	return cell_ref(hash_void);
    }
    return value;
}

// TODO check is this is right...
static cell *apply_lambda(cell *fun, cell* args, cell *env) {
    cell *result = NIL; // TODO should be void
    cell *body;
    assert(fun->type == c_LAMBDA);
    // add one level to environment

    // pick up arguments one by one and add to assoc
    {
	cell *nam;
        cell *assoc = cell_assoc();
        cell *argnames = cell_ref(fun->_.cons.car);
        while (cell_split(argnames, &nam, &argnames)) {
            cell *val;
	    assert(cell_is_symbol(nam));
            if (!cell_split(args, &val, &args)) {
                // TODO if more than one, have one message
		cell_unref(error_rt1("missing value for", cell_ref(nam)));
                val = cell_ref(hash_void);
            } else {
                val = eval(val, env);
            }
            // TODO start with assoc = NIL
            assoc_set(assoc, nam, val);
        }
        if (args != NIL) {
            cell_unref(error_rt1("excess arguments ignored", args));
        }
        // add one level of environment
        env = cell_cons(assoc, env);
    }
    {
        cell *expr;
	body = cell_ref(fun->_.cons.cdr);
        cell_unref(fun);

        // evaluate one expression at a time
        while (cell_split(body, &expr, &body)) {
            cell_unref(result);
            result = eval(expr, env);
        }
        // TODO end recursion
    }
    // drop one level of environment
    cell_split(env, (cell **)0, &env);
    return verify_nil(body, result);
}

cell *eval(cell *arg, cell* env) {

    if (arg) switch (arg->type) {
    default:        // value is itself
	return arg;

    case c_SYMBOL:  // evaluate symbol
	{
	    cell *val;
	    while (env) {
		assert(cell_is_cons(env));
		if (assoc_get(env->_.cons.car, arg, &val)) {
		    cell_unref(arg);
		    return val;
		}
		env = env->_.cons.cdr;
	    }
	    // global
	    val = cell_ref(arg->_.symbol.val);
	    cell_unref(arg);
	    return val;
	}

    case c_CONS:    
        {
            cell *fun;
	    struct cell_s *(*def)(struct cell_s *, struct cell_s *);
	    cell_split(arg, &fun, &arg);
	    fun = eval(fun, env);
	    // TODO may consider moving evaluation out of functions

	    switch (fun ? fun->type : c_CONS) {
            case c_CFUN:
                def = fun->_.cfun.def;
                cell_unref(fun);
		return (*def)(arg, env);

	    case c_LAMBDA:
                // TODO perhaps
                return apply_lambda(fun, arg, env);

	    default: // not a function
		// TODO show item before eval
		cell_unref(arg);
		return error_rt1("not a function", fun);
	    }
        }
    }
    assert(0);
    return NIL;
}
