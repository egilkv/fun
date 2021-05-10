/* TAB P
 *
 * TODO should evaluation happen in functions? perhaps
 */

#include <assert.h>

#include "cfun.h"
#include "oblist.h"
#include "err.h"

cell *hash_assoc;
cell *hash_defq;
cell *hash_div;
cell *hash_eval;
cell *hash_f;
cell *hash_if;
cell *hash_lambda;
cell *hash_list;
cell *hash_lt;
cell *hash_minus;
cell *hash_not;
cell *hash_plus;
cell *hash_quote;
cell *hash_ref;
cell *hash_t;
cell *hash_times;
cell *hash_vector;
cell *hash_void;

// function with 1 argument
static int arg1(cell *args, cell **a1p) {
    if (cell_split(args, a1p, &args)) {
        if (args) {
	    cell_unref(error_rt1("excess argument(s) ignored", args));
        }
        return 1;
    } else {
        *a1p = NIL;
        if (args == NIL) {
	    cell_unref(error_rt0("missing argument"));
        } else {
	    cell_unref(error_rt1("malformed argument(s)", args));
        }
        return 0;
    }
}

// function with 2 arguments
static int arg2(cell *args, cell **a1p, cell **a2p) {
    if (cell_split(args, a1p, &args)) {
        if (cell_split(args, a2p, &args)) {
            if (args) {
		cell_unref(error_rt1("excess argument(s) ignored", args));
            }
            return 1;
        }
        *a2p = NIL;
    }
    *a1p = NIL;
    if (args == NIL) {
	cell_unref(error_rt0("missing argument(s)"));
    } else {
	cell_unref(error_rt1("malformed argument(s)", args));
    }
    return 0;
}

// a in always unreffed
// dump is unreffed only if error
static int eval_numeric(cell *a, integer_t *valuep, cell *dump, environment *env) {
    if ((a = eval(a, env))) {
        switch (a->type) {
        case c_INTEGER:
            *valuep = a->_.ivalue;
            cell_unref(a);
            return 1;
	default:
	    break;
        }
    }
    cell_unref(dump);
    cell_unref(error_rt1("not a number", a));
    return 0;
}

static int eval_index(cell *a, index_t *indexp, cell *dump, environment *env) {
    integer_t value;
    if (!eval_numeric(a, &value, dump, env)) return 0;
    if (value < 0) {
	cell_unref(dump);
	cell_unref(error_rti("cannot be negative", value));
	return 0;
    }
    *indexp = (index_t)value;
    return 1;
}

// a in always unreffed
// dump is unreffed only if error
static int pick_boolean(cell *a, int *boolp, cell *dump) {
    if (a == hash_t) {
        *boolp = 1;
        cell_unref(a);
        return 1;
    } else if (a == hash_f) {
        *boolp = 0;
        cell_unref(a);
        return 1;
    }
    cell_unref(dump);
    cell_unref(error_rt1("not a boolean", a));
    return 0;
}

static cell *cfun_defq(cell *args, environment *env) {
    cell *a, *b;
    if (!arg2(args, &a, &b)) {
        cell_ref(hash_void); // error
    }
    if (!cell_is_symbol(a)) {
        cell_unref(b);
        return error_rt1("not a symbol", a);
    }
    b = eval(b, env);
    if (env) {
	assert(env->assoc);
	if (!assoc_set(env->assoc, a, cell_ref(b))) {
	    cell_unref(b);
	    cell_unref(error_rt1("cannot redefine immutable", a));
	}
    } else {
	cell_unref(a->_.symbol.val);
	a->_.symbol.val = cell_ref(b);
	cell_unref(a);
    }
    return b;
}

static cell *cfun_not(cell *args, environment *env) {
    cell *a;
    int bool;
    if (arg1(args, &a)) {
	a = eval(a, env);
        if (pick_boolean(a, &bool, NIL)) {
	    return bool ? cell_ref(hash_f) : cell_ref(hash_t);
        }
    }
    return cell_ref(hash_void); // error
}

static cell *cfun_if(cell *args, environment *env) {
    int bool;
    cell *a;
    if (cell_split(args, &a, &args)) {
	a = eval(a, env);
        if (!pick_boolean(a, &bool, args)) return cell_ref(hash_void); // error
    }
    // second argument must be present
    if (!cell_split(args, &a, &args)) {
	return error_rt1("missing body for if statement", args); // TODO rephrase
    }
    if (bool) { // true?
        cell_unref(args);
	return eval(a, env);
    } 
    // else
    cell_unref(a);
    if (!args) {
        // no else part given, value is void
        return cell_ref(hash_void);
    } else if (!cell_split(args, &a, &args)) {
	return error_rt1("missing body for else statement", args); // TODO rephrase
    }
    if (args) {
	cell_unref(error_rt1("extra argument for if ignored", args));
    }
    // finally, else proper
    return eval(a, env);
}

static cell *cfun_quote(cell *args, environment *env) {
    cell *a;
    if (arg1(args, &a)) {
	return a;
    }
    return cell_ref(hash_void); // error
}

static cell *cfun_lambda(cell *args, environment *env) {
    cell *arglist;
    cell *cp;
    if (!cell_split(args, &arglist, &args)) {
        return error_rt1("Missing function argument list", args);
    }
    cp = cell_cons(arglist, args);
    cp->type = c_LAMBDA;
    return cp;
}

static cell *cfun_plus(cell *args, environment *env) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    while (cell_split(args, &a, &args)) {
	if (!eval_numeric(a, &operand, args, env)) return cell_ref(hash_void); // error
        result += operand; // TODO overflow etc
    }
    return verify_nil(args, cell_integer(result));
}

static cell *cfun_minus(cell *args, environment *env) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    if (cell_split(args, &a, &args)) {
	if (!eval_numeric(a, &result, args, env)) return cell_ref(hash_void); // error
	if (args == NIL) {
            // special case, one argument
            result = -result; // TODO overflow
        }
    }
    while (cell_split(args, &a, &args)) {
	if (!eval_numeric(a, &operand, args, env)) return cell_ref(hash_void);
        result -= operand; // TODO overflow etc
    }
    return verify_nil(args, cell_integer(result));
}

static cell *cfun_times(cell *args, environment *env) {
    integer_t result = 1;
    integer_t operand;
    cell *a;
    while (cell_split(args, &a, &args)) {
	if (!eval_numeric(a, &operand, args, env)) return cell_ref(hash_void);
        result *= operand; // TODO overflow etc
    }
    return verify_nil(args, cell_integer(result));
}

static cell *cfun_div(cell *args, environment *env) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    // TODO must have two arguments?
    if (cell_split(args, &a, &args)) {
	if (!eval_numeric(a, &result, args, env)) return cell_ref(hash_void); // error
    }
    while (cell_split(args, &a, &args)) {
	if (!eval_numeric(a, &operand, args, env)) return cell_ref(hash_void); // error
        if (operand == 0) {
            cell_unref(args);
	    return error_rt0("attempted division by zero");
        }
        // TODO should rather create a quotient
        result /= operand;
    }
    return verify_nil(args, cell_integer(result));
}

static cell *cfun_lt(cell *args, environment *env) {
    integer_t value;
    integer_t operand;
    cell *a;
    if (cell_split(args, &a, &args)) {
	if (!eval_numeric(a, &value, args, env)) return cell_ref(hash_void); // error
    }
    while (cell_split(args, &a, &args)) {
	if (!eval_numeric(a, &operand, args, env)) return cell_ref(hash_void); // error
	if (value < operand) { // condition satisfied?
            value = operand;
	} else {
	    cell_unref(args);
	    return cell_ref(hash_f); // false
	}
    }
    return verify_nil(args, cell_ref(hash_t));
}

static cell *cfun_eval(cell *args, environment *env) {
    cell *a;
    if (!arg1(args, &a)) return cell_ref(hash_void);
    return eval(a, env);
}

static cell *cfun_list(cell *args, environment *env) {
    cell *list = NIL;
    cell **pp = &list;
    cell *a;
    while (cell_split(args, &a, &args)) {
	*pp = cell_cons(eval(a, env), NIL);
	pp = &((*pp)->_.cons.cdr);
    }
    cell_unref(verify_nil(args, NIL));
    return list;
}

static cell *cfun_vector(cell *args, environment *env) {
    cell *length;
    cell *vector;
    cell *a;
    index_t len;
    if (!cell_split(args, &length, &args)) {
	return error_rt1("missing vector length", args);
    }
    if (length) {
        // vector of specified length
	// TODO index_t check > 0
	index_t index = 0;
	if (!eval_index(length, &len, args, env)) return cell_ref(hash_void); // error
        vector = cell_vector(len);

        while (cell_split(args, &a, &args)) {
            if (!vector_set(vector, index, eval(a, env))) {
                // TODO should include a
                cell_unref(error_rt1("excess initialization data ignored", args));
                cell_unref(args);
                args = NIL;
                break;
            }
            ++index;
        }
        cell_unref(verify_nil(args, NIL));
        if (index < len) {
            // TODO repeat initialization???
        }
    } else {
        // vector of unknown length
        len = 0;
        vector = cell_vector(0);
        // TODO rather inefficient
        while (cell_split(args, &a, &args)) {
            vector_resize(vector, ++len);
            if (!vector_set(vector, len-1, eval(a, env))) {
                assert(0); // out of bounds should not happen
            }
        }
        cell_unref(verify_nil(args, NIL));
    }
    return vector;
}

static cell *cfun_assoc(cell *args, environment *env) {
    cell *assoc;
    assoc = cell_assoc(); // empty
#if 0 // TODO implement
    cell *length;
    cell *a;
    index_t len;
    if (!cell_split(args, &length, &args)) {
	return error_rt1("missing vector length", args);
    }
    if (length) {
        // vector of specified length
	// TODO index_t check > 0
	index_t index = 0;
	if (!eval_index(length, &len, args, env)) return cell_ref(hash_void); // error
        vector = cell_vector(len);

        while (cell_split(args, &a, &args)) {
            if (!vector_set(vector, index, eval(a, env))) {
                // TODO should include a
                cell_unref(error_rt1("excess initialization data ignored", args));
                cell_unref(args);
                args = NIL;
                break;
            }
            ++index;
        }
        cell_unref(verify_nil(args, NIL));
        if (index < len) {
            // TODO repeat initialization???
        }
    } else {
        // vector of unknown length
        len = 0;
        vector = cell_vector(0);
        // TODO rather inefficient
        while (cell_split(args, &a, &args)) {
            vector_resize(vector, ++len);
            if (!vector_set(vector, len-1, eval(a, env))) {
                assert(0); // out of bounds should not happen
            }
        }
    }
#endif
    cell_unref(verify_nil(args, NIL));
    return assoc;
}

static cell *cfun_ref(cell *args, environment *env) {
    cell *a, *b;
    cell *value;
    index_t index;
    if (!arg2(args, &a, &b)) {
        cell_ref(hash_void); // error
    }
    a = eval(a, env);
    if (a) switch (a->type) {
    case c_VECTOR:
	if (!eval_index(b, &index, NIL, env)) return cell_ref(hash_void); // error
	if (!vector_get(a, index, &value)) {
	    cell_unref(a);
	    return error_rti("vector index out of bounds", index);
	}
	cell_unref(a);
	return value;

    case c_ASSOC:
	{
	    cell *value;
	    if (!assoc_get(a, eval(b, env), &value)) {
		cell_unref(a);
		return error_rt1("assoc key does not exist", b);
	    }
	    cell_unref(a);
	    return value;
	}

    case c_CONS:
	{
	    index_t i = 0;
	    value = NIL;
	    if (!eval_index(b, &index, NIL, env)) return cell_ref(hash_void); // error
	    do {
		cell_unref(value);
		if (a == NIL) {
		    return error_rti("list index out of bounds", index);
		}
		if (!cell_split(a, &value, &a)) {
		    return error_rt1("not a proper list", a);
		}
	    } while (i++ < index);
	}
	cell_unref(a);
        return value;

    case c_LAMBDA:
    case c_CFUN:
    // TODO ref should work for functions ??
    default:
	    break;
    }
    cell_unref(b);
    return error_rt1("cannot referrence", a);
    return 0;

}

void cfun_init() {
    (hash_assoc  = oblist("#assoc"))  ->_.symbol.val = cell_cfun(cfun_assoc);
    (hash_defq   = oblist("#defq"))   ->_.symbol.val = cell_cfun(cfun_defq);
    (hash_div    = oblist("#div"))    ->_.symbol.val = cell_cfun(cfun_div);
    (hash_eval   = oblist("#eval"))   ->_.symbol.val = cell_cfun(cfun_eval);
    (hash_if     = oblist("#if"))     ->_.symbol.val = cell_cfun(cfun_if);
    (hash_lambda = oblist("#lambda")) ->_.symbol.val = cell_cfun(cfun_lambda);
    (hash_lt     = oblist("#lt"))     ->_.symbol.val = cell_cfun(cfun_lt);
    (hash_list   = oblist("#"))       ->_.symbol.val = cell_cfun(cfun_list);
    (hash_minus  = oblist("#minus"))  ->_.symbol.val = cell_cfun(cfun_minus);
    (hash_not    = oblist("#not"))    ->_.symbol.val = cell_cfun(cfun_not);
    (hash_plus   = oblist("#plus"))   ->_.symbol.val = cell_cfun(cfun_plus);
    (hash_quote  = oblist("#quote"))  ->_.symbol.val = cell_cfun(cfun_quote);
    (hash_ref    = oblist("#ref"))    ->_.symbol.val = cell_cfun(cfun_ref);
    (hash_times  = oblist("#times"))  ->_.symbol.val = cell_cfun(cfun_times);
    (hash_vector = oblist("#vector")) ->_.symbol.val = cell_cfun(cfun_vector);

    // TODO value should be themselves
    hash_f       = oblist("#f");
    hash_t       = oblist("#t");
    hash_void    = oblist("#void");
}




