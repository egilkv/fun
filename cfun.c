/* TAB P
 *
 * TODO should evaluation happen in functions? perhaps
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cfun.h"
#include "oblist.h"
#include "err.h"

cell *hash_amp;
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
cell *hash_refq;
cell *hash_t;
cell *hash_times;
cell *hash_vector;
cell *hash_void;

// function with 1 argument
static int arg1(cell *args, cell **a1p) {
    if (list_split(args, a1p, &args)) {
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
    if (list_split(args, a1p, &args)) {
	if (list_split(args, a2p, &args)) {
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
static int get_numeric(cell *a, integer_t *valuep, cell *dump) {
    if (a) switch (a->type) {
    case c_INTEGER:
	*valuep = a->_.ivalue;
	cell_unref(a);
	return 1;
    default:
	break;
    }
    cell_unref(dump);
    cell_unref(error_rt1("not a number", a));
    return 0;
}

static int get_index(cell *a, index_t *indexp, cell *dump) {
    integer_t value;
    if (!get_numeric(a, &value, dump)) return 0;
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
        return cell_ref(hash_void); // error
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
#if 1
    cell *a, *b;
    if (!arg2(args, &a, &b)) {
        return cell_ref(hash_void); // error
    }
    a = eval(a, env);
    if (!pick_boolean(a, &bool, b)) {
        return cell_ref(hash_void); // error
    }
    if (cell_is_pair(b)) { // if/else present
        if (bool) {
            a = eval(cell_ref(cell_car(b)), env);
        } else {
            a = eval(cell_ref(cell_cdr(b)), env);
        }
        cell_unref(b);

    } else if (bool) {
        a = b;
    } else {
        cell_unref(b);
        a = cell_ref(hash_void);
    }
    return a;
#else
    cell *a;
    if (list_split(args, &a, &args)) {
	a = eval(a, env);
        if (!pick_boolean(a, &bool, args)) return cell_ref(hash_void); // error
    }
    // second argument must be present
    if (!list_split(args, &a, &args)) {
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
    } else if (!list_split(args, &a, &args)) {
	return error_rt1("missing body for else statement", args); // TODO rephrase
    }
    if (args) {
	cell_unref(error_rt1("extra argument for if ignored", args));
    }
    // finally, else proper
    return eval(a, env);
#endif
}

static cell *cfun_quote(cell *args, environment *env) {
    cell *a;
    if (!arg1(args, &a)) {
        return cell_ref(hash_void); // error
    }
    return a;
}

static cell *cfun_lambda(cell *args, environment *env) {
    cell *arglist;
    cell *cp;
    if (!list_split(args, &arglist, &args)) {
        return error_rt1("Missing function argument list", args);
    }
    cp = cell_list(arglist, args);
    cp->type = c_LAMBDA;
    return cp;
}

static cell *cfun_plus(cell *args, environment *env) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    while (list_split(args, &a, &args)) {
	if (!get_numeric(eval(a, env), &operand, args)) return cell_ref(hash_void); // error
        result += operand; // TODO overflow etc
    }
    return verify_nil(args, cell_integer(result));
}

static cell *cfun_minus(cell *args, environment *env) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    if (list_split(args, &a, &args)) {
	if (!get_numeric(eval(a, env), &result, args)) return cell_ref(hash_void); // error
	if (args == NIL) {
            // special case, one argument
            result = -result; // TODO overflow
        }
    }
    while (list_split(args, &a, &args)) {
	if (!get_numeric(eval(a, env), &operand, args)) return cell_ref(hash_void);
        result -= operand; // TODO overflow etc
    }
    return verify_nil(args, cell_integer(result));
}

static cell *cfun_times(cell *args, environment *env) {
    integer_t result = 1;
    integer_t operand;
    cell *a;
    while (list_split(args, &a, &args)) {
	if (!get_numeric(eval(a, env), &operand, args)) return cell_ref(hash_void);
        result *= operand; // TODO overflow etc
    }
    return verify_nil(args, cell_integer(result));
}

static cell *cfun_div(cell *args, environment *env) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    // TODO must have two arguments?
    if (list_split(args, &a, &args)) {
	if (!get_numeric(eval(a, env), &result, args)) return cell_ref(hash_void); // error
    }
    while (list_split(args, &a, &args)) {
	if (!get_numeric(eval(a, env), &operand, args)) return cell_ref(hash_void); // error
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
    if (list_split(args, &a, &args)) {
	if (!get_numeric(eval(a, env), &value, args)) return cell_ref(hash_void); // error
    }
    while (list_split(args, &a, &args)) {
	if (!get_numeric(eval(a, env), &operand, args)) return cell_ref(hash_void); // error
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
    while (list_split(args, &a, &args)) {
	*pp = cell_list(eval(a, env), NIL);
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
    if (!list_split(args, &length, &args)) {
	return error_rt1("missing vector length", args);
    }
    if (length) {
        // vector of specified length
	// TODO index_t check > 0
	index_t index = 0;
	if (!get_index(eval(length, env), &len, args)) return cell_ref(hash_void); // error
        vector = cell_vector(len);

        // TODO can be optimized
	while (list_split(args, &a, &args)) {
            if (!vector_set(vector, index, eval(a, env))) {
		cell_unref(error_rti("excess initialization data ignored at", index));
                cell_unref(args);
                args = NIL;
                break;
            }
            ++index;
        }
        cell_unref(verify_nil(args, NIL));
        if (index < len) { // need to pad rest of vector with last element?
            index_t last_index = index-1;
            vector_get(vector, last_index, &a);
            do {
                vector_set(vector, index, cell_ref(a));
            } while (++index < len);
            cell_unref(a);
        }
    } else {
        // vector of unknown length
        len = 0;
        vector = cell_vector(0);
        // TODO rather inefficient
	while (list_split(args, &a, &args)) {
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
    cell *a;
    cell *b;
    cell *assoc = cell_assoc();
    while (list_split(args, &a, &args)) {
        if (!pair_split(a, &a, &b)) {
	    cell_unref(error_rt1("initialization item not in form of key colon value", a));
        } else {
            b = eval(b, env);
	    if (!assoc_set(assoc, a, b)) {
		cell_unref(b);
		cell_unref(error_rt1("duplicate key ignored", a));
	    }
	}
    }
    cell_unref(verify_nil(args, NIL));
    return assoc;
}

static cell *cfun_amp(cell *args, environment *env) {
    cell *a;
    cell *result = NIL;
    while (list_split(args, &a, &args)) {
        a = eval(a, env);
        switch (a ? a->type : c_LIST) {

        case c_STRING:
            if (result == NIL) {
                result = a; // TODO somewhat cheeky
            } else if (!cell_is_string(result)) {
                cell_unref(result);
                cell_unref(args);
		return error_rt1("& applied to non-string and string", a);
            } else {
                // TODO handle alen or rlen zero
                // TODO optimize for case of more than two arguments
		index_t rlen = result->_.string.len;
		index_t alen = a->_.string.len;
		char_t *newstr = malloc(rlen + alen + 1);
                assert(newstr);
		if (rlen > 0) memcpy(newstr, result->_.string.ptr, rlen);
		if (alen > 0) memcpy(newstr+rlen, a->_.string.ptr, alen);
                newstr[rlen+alen] = '\0';
                cell_unref(result);
                cell_unref(a);
		result = cell_astring(newstr, rlen+alen);
            }
            break;

        case c_VECTOR:
            cell_unref(result);
            cell_unref(args);
	    return error_rt1("vector & not yet implemented", a);

        case c_ASSOC:
            cell_unref(result);
            cell_unref(args);
	    return error_rt1("assoc & not yet implemented", a);

        case c_LIST:
            if (result == NIL) {
                result = a;
            } else {
                cell_unref(result);
                cell_unref(args);
		return error_rt1("list & not yet implemented", a);
            }
            break;

        default:
            cell_unref(result);
            cell_unref(args);
	    return error_rt1("operator & cannot be applied", a);
        }
    }
    cell_unref(verify_nil(args, NIL));
    return result;
}

static cell *common_ref(cell *a, cell *b) {
    cell *value;
    index_t index;
    if (a) switch (a->type) {

    case c_VECTOR:
	if (!get_index(b, &index, NIL)) return cell_ref(hash_void); // error
	if (!vector_get(a, index, &value)) {
            value = error_rti("vector index out of bounds", index);
	}
	cell_unref(a);
	return value;

    case c_STRING:
	if (!get_index(b, &index, NIL)) return cell_ref(hash_void); // error
	if (index < a->_.string.len) {
	    char_t *s = malloc(1 + 1);
	    assert(s);
	    s[0] = a->_.string.ptr[index];
	    s[1] = '\0';
	    value = cell_astring(s, 1);
        } else {
            return error_rti("string index out of bounds", index);
	}
	cell_unref(a);
	return value;

    case c_ASSOC:
	{
	    cell *value;
	    if (!assoc_get(a, b, &value)) {
		cell_unref(a);
		return error_rt1("assoc key does not exist", b);
	    }
	    cell_unref(a);
	    return value;
	}

    case c_LIST:
	{
	    index_t i = 0;
	    value = NIL;
	    if (!get_index(b, &index, NIL)) return cell_ref(hash_void); // error
	    do {
		cell_unref(value);
		if (a == NIL) {
		    return error_rti("list index out of bounds", index);
		}
		if (!list_split(a, &value, &a)) {
		    return error_rt1("not a proper list", a);
		}
	    } while (i++ < index);
	}
	cell_unref(a);
        return value;

    case c_PAIR:
	if (!get_index(b, &index, NIL)) return cell_ref(hash_void); // error
	switch (index) {
	case 0:
	    value = cell_ref(cell_car(a));
	    break;
	case 1:
	    value = cell_ref(cell_cdr(a));
	    break;
	default:
	    value = error_rti("pair index out of bounds", index);
	    break;
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

static cell *cfun_ref(cell *args, environment *env) {
    cell *a, *b;
    if (!arg2(args, &a, &b)) {
        return cell_ref(hash_void); // error
    }
    return common_ref(eval(a, env), eval(b, env));
}

static cell *cfun_refq(cell *args, environment *env) {
    cell *a, *b;
    if (!arg2(args, &a, &b)) {
        return cell_ref(hash_void); // error
    }
    return common_ref(eval(a, env), b);
}

void cfun_init() {
    (hash_amp    = oblist("#amp"))    ->_.symbol.val = cell_cfun(cfun_amp);
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
    (hash_refq   = oblist("#refq"))   ->_.symbol.val = cell_cfun(cfun_refq);
    (hash_times  = oblist("#times"))  ->_.symbol.val = cell_cfun(cfun_times);
    (hash_vector = oblist("#vector")) ->_.symbol.val = cell_cfun(cfun_vector);

    // TODO value should be themselves
    hash_f       = oblist("#f");
    hash_t       = oblist("#t");
    hash_void    = oblist("#void");
}




