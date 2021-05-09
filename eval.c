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
static cell *apply(cell *fun, cell* arglist, cell *env) {
    cell *body;
    cell *expr;
    cell *result = NIL; // TODO should be void...
    //TODO assuming def is a lambda
    assert(fun->type == c_LAMBDA);
    body = cell_ref(fun->_.cons.cdr);
    cell_unref(fun);
    cell_unref(arglist);

    // evaluate one expression at a time
    while (cell_split(body, &expr, &body)) {
        cell_unref(result);
	result = eval(expr, env);
    }
    return verify_nil(body, result);
}

cell *eval(cell *arg, cell* env) {

    if (arg) switch (arg->type) {
    default:        // value is itself
	return arg;

    case c_SYMBOL:  // evaluate symbol
	return cell_ref(arg->_.symbol.val);

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
		return apply(fun, arg, env);

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
