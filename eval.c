/* TAB P
 *
 * TODO should evaluation happen in functions? perhaps
 */

#include <assert.h>
#include <stdlib.h>

#include "cfun.h"
#include "cell.h"
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
void apply_lambda(cell *fun, cell* args, environment **envp) {
    assert(fun->type == c_LAMBDA);
    struct env_s *newenv;
    cell *nam;
    cell *val;
    cell *argnames = cell_ref(fun->_.cons.car);
    // add one level to environment
    newenv = malloc(sizeof(environment));
    newenv->prev = *envp;
    newenv->assoc = cell_assoc();
    newenv->prog = cell_ref(fun->_.cons.cdr);
    cell_unref(fun);

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
	if (!assoc_set(newenv->assoc, nam, val)) {
	    cell_unref(val);
	    cell_unref(error_rt1("duplicate parameter name, value ignored", nam));
	}
    }
    if (args != NIL) {
	cell_unref(error_rt1("excess arguments ignored", args));
    }
    // add one level of environment
    *envp = newenv;
}

cell *eval(cell *arg, environment *env) {

    if (arg) switch (arg->type) {
    default:        // value is itself
	return arg;

    case c_SYMBOL:  // evaluate symbol
	{
	    cell *val;
	    while (env) {
		if (assoc_get(env->assoc, arg, &val)) {
		    cell_unref(arg);
		    return val;
		}
		env = env->prev;
	    }
	    // global
	    val = cell_ref(arg->_.symbol.val);
	    cell_unref(arg);
	    return val;
	}

    case c_LIST:
        {
            cell *fun;
	    struct cell_s *(*def)(struct cell_s *, struct env_s *);
	    list_split(arg, &fun, &arg);
	    fun = eval(fun, env);
	    // TODO may consider moving evaluation out of functions

            switch (fun ? fun->type : c_LIST) {
            case c_CFUN:
                def = fun->_.cfun.def;
                cell_unref(fun);
		return (*def)(arg, env);

	    case c_LAMBDA:
                // TODO perhaps
		apply_lambda(fun, arg, &env);

		// run program
		assert(env != NULL);
		{
		    cell *result = NIL; // TODO void
		    cell *expr;

		    // evaluate one expression at a time
		    while (list_split(env->prog, &expr, &(env->prog))) {
			cell_unref(result);
			// TODO C recursion
			result = eval(expr, env);
		    }
		    // TODO end recursion

		    // drop one level of environment
		    {
			environment *prevenv = env->prev;
			cell_unref(env->assoc);
			cell_unref(verify_nil(env->prog, NIL));
			free(env);
			env = prevenv;
		    }
		    return result;
		}

	    default: // not a function
		// TODO show item before eval
		cell_unref(arg);
		return error_rt1("not a function", fun);
	    }
        }

    case c_PAIR:
        return error_rt1("pair cannot be evaluated", arg);
    }
    assert(0);
    return NIL;
}
