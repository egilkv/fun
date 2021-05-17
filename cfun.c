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
cell *hash_args;
cell *hash_assoc;
cell *hash_defq;
cell *hash_div;
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
cell *hash_use;
cell *hash_vector;
cell *hash_void;

// function with 0 arguments
void arg0(cell *args) {
    if (args) {
	cell_unref(error_rt1("excess argument(s) ignored", args));
    }
}

// function with 3 arguments
// if false, *ap is error value
int arg3(cell *args, cell **ap, cell **bp, cell **cp) {
    *ap = NIL;
    if (!list_split(args, ap, &args)) {
	assert(args == NIL);
	*ap = error_rt0("missing 1st argument");
	return 0;
    }
    if (bp && !list_split(args, bp, &args)) {
	assert(args == NIL);
	cell_unref(*ap);
	*ap = error_rt0("missing 2nd argument");
	return 0;
    }
    if (cp && !list_split(args, cp, &args)) {
	assert(args == NIL);
	cell_unref(*ap);
	cell_unref(*bp);
	*ap = error_rt0("missing 3rd argument");
	return 0;
    }
    arg0(args);
    return 1;
}

// function with 1 argument
// if false, *ap is error value
int arg1(cell *args, cell **ap) {
    return arg3(args, ap, NULL, NULL);
}

// function with 2 arguments
// if false, *ap is error value
int arg2(cell *args, cell **ap, cell **bp) {
    return arg3(args, ap, bp, NULL);
}

// a in always unreffed
// dump is unreffed only if error
int get_integer(cell *a, integer_t *valuep, cell *dump) {
    if (!cell_is_integer(a)) {
	cell_unref(dump);
	cell_unref(error_rt1("not a number", a));
	return 0;
    }
    *valuep = a->_.ivalue;
    cell_unref(a);
    return 1;
}

int get_index(cell *a, index_t *indexp, cell *dump) {
    integer_t value;
    if (!get_integer(a, &value, dump)) return 0;
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
int get_string(cell *a, char_t **valuep, index_t *lengthp, cell *dump) {
    if (a) switch (a->type) {
    case c_STRING:
	*valuep = a->_.string.ptr;
        *lengthp = a->_.string.len;
	cell_unref(a);
	return 1;
    default:
	break;
    }
    cell_unref(dump);
    cell_unref(error_rt1("not a string", a));
    return 0;
}

// a in always unreffed
// dump is unreffed only if error
int get_symbol(cell *a, char_t **valuep, cell *dump) {
    if (a) switch (a->type) {
    case c_SYMBOL:
	*valuep = a->_.symbol.nam;
	cell_unref(a);
	return 1;
    default:
	break;
    }
    cell_unref(dump);
    cell_unref(error_rt1("not a symbol", a));
    return 0;
}

// as get_string, but nul-terminated C string is returned
int get_cstring(cell *a, char **valuep, cell *dump) {
    index_t dummy;
    return get_string(a, valuep, &dummy, dump);
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

static cell *cfunQ_defq(cell *args, environment *env) {
    cell *a, *b;
    if (!arg2(args, &a, &b)) {
	return a; // error
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
	// TODO mutable
	oblist_set(a, cell_ref(b));
	cell_unref(a);
    }
    return b;
}

static cell *cfun1_not(cell *a) {
    int bool;
    if (!pick_boolean(a, &bool, NIL)) {
	return cell_ref(hash_void); // error
    }
    return bool ? cell_ref(hash_f) : cell_ref(hash_t);
}

static cell *cfunQ_if(cell *args, environment *env) {
    int bool;
    cell *a, *b;
    if (!arg2(args, &a, &b)) {
	return a; // error
    }
    a = eval(a, env);
    if (!pick_boolean(a, &bool, b)) {
        return cell_ref(hash_void); // error
    }
    if (bool) {
	if (!pair_split(b, &a, (cell **)0)) {
            // no else-part
	    a = b;
        }
    } else {
	if (!pair_split(b, (cell **)0, &a)) {
            // no else-part
            cell_unref(b);
            return cell_ref(hash_void);
        }
    }
    if (env) {
        // evalutae in-line
	env->prog = cell_list(a, env->prog);
        return NIL;
    } else {
        return eval(a, env);
    }
}

static cell *cfunQ_quote(cell *args, environment *env) {
    cell *a;
    arg1(args, &a); // sets void if error
    return a;
}

static cell *cfunQ_lambda(cell *args, environment *env) {
    cell *arglist;
    cell *cp;
    if (!list_split(args, &arglist, &args)) {
        return error_rt1("Missing function argument list", args);
    }
    cp = cell_list(arglist, args);
    cp->type = c_LAMBDA;
    return cp;
}

static cell *cfunN_plus(cell *args) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    while (list_split(args, &a, &args)) {
        if (!get_integer(a, &operand, args)) return cell_ref(hash_void); // error
        result += operand; // TODO overflow etc
    }
    assert(args == NIL);
    return cell_integer(result);
}

static cell *cfunN_minus(cell *args) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    if (list_split(args, &a, &args)) {
        if (!get_integer(a, &result, args)) return cell_ref(hash_void); // error
	if (args == NIL) {
            // special case, one argument
            result = -result; // TODO overflow
        }
    }
    while (list_split(args, &a, &args)) {
        if (!get_integer(a, &operand, args)) return cell_ref(hash_void);
        result -= operand; // TODO overflow etc
    }
    assert(args == NIL);
    return cell_integer(result);
}

static cell *cfunN_times(cell *args) {
    integer_t result = 1;
    integer_t operand;
    cell *a;
    while (list_split(args, &a, &args)) {
        if (!get_integer(a, &operand, args)) return cell_ref(hash_void);
        result *= operand; // TODO overflow etc
    }
    assert(args == NIL);
    return cell_integer(result);
}

static cell *cfun2_div(cell *a, cell *b) {
    integer_t result;
    integer_t operand;
    if (!get_integer(a, &result, b)) return cell_ref(hash_void); // error
    if (!get_integer(b, &operand, NIL)) return cell_ref(hash_void); // error

    if (operand == 0) {
        return error_rt0("attempted division by zero");
    }
    // TODO div mod quotient
    result /= operand;
    return cell_integer(result);
}

static cell *cfunN_lt(cell *args) {
    int argno = 0;
    integer_t value;
    integer_t operand;
    cell *a;
    // TODO should be cfunQ_lt, do not evaluate more than necessary
    while (list_split(args, &a, &args)) {
        if (!get_integer(a, &operand, args)) return cell_ref(hash_void); // error
	if (argno++ == 0 || value < operand) { // condition satisfied?
	    value = operand;
	} else {
	    cell_unref(args);
	    return cell_ref(hash_f); // false
	}
    }
    assert(args == NIL);
    return cell_ref(hash_t);
}

static cell *cfunN_list(cell *args) {
    return args;
}

static cell *cfunQ_vector(cell *args, environment *env) {
    cell *vector;
    index_t len;
    cell *a;
    // vector of length determined by initialization
    len = 0;
    vector = cell_vector(0);
    // TODO rather inefficient
    while (list_split(args, &a, &args)) {
        if (cell_is_pair(a)) {  // index : value
	    index_t index;
            cell *b;
	    pair_split(a, &a, &b);
	    if (get_index(eval(a, env), &index, b)) {
                if (index >= len) {
                    len = index+1;
                    vector_resize(vector, len);
                }
                // TODO check if redefining, which is not allowed
		if (!vector_set(vector, index, eval(b, env))) {
                    assert(0); // out of bounds should not happen
                }
            }
        } else {
            vector_resize(vector, ++len);
            if (!vector_set(vector, len-1, eval(a, env))) {
                assert(0); // out of bounds should not happen
            }
        }
    }
    assert(args == NIL);
    return vector;
}

static cell *cfunQ_assoc(cell *args, environment *env) {
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
    assert(args == NIL);
    return assoc;
}

static cell *cfunN_amp(cell *args) {
    cell *a;
    cell *result = NIL;
    while (list_split(args, &a, &args)) {
	switch (a ? a->type : c_LIST) {

        case c_STRING:
            if (result == NIL) {
                result = a; // TODO somewhat cheeky
            } else if (!cell_is_string(result)) {
                cell_unref(result);
                cell_unref(args);
		return error_rt1("& applied to non-string and string", a);
            } else {
                // TODO optimize for case of more than two arguments
		index_t rlen = result->_.string.len;
		index_t alen = a->_.string.len;
		if (alen == 0) {
		    cell_unref(a);
		} else if (rlen == 0) {
		    cell_unref(result);
		    result = a;
		} else {
		    char_t *newstr = malloc(rlen + alen + 1);
		    assert(newstr);
		    memcpy(newstr, result->_.string.ptr, rlen);
		    memcpy(newstr+rlen, a->_.string.ptr, alen);
		    newstr[rlen+alen] = '\0';
		    cell_unref(result);
		    cell_unref(a);
		    result = cell_astring(newstr, rlen+alen);
		}
            }
            break;

        case c_VECTOR:
            cell_unref(result);
            cell_unref(args);
	    return error_rt1("vector & not yet implemented", a);

        case c_ASSOC:
            if (result == NIL) {
                result = a; // TODO somewhat cheeky
	    } else if (!cell_is_assoc(result)) {
                cell_unref(result);
                cell_unref(args);
		return error_rt1("& applied to non-assoc and assoc", a);
            } else {
		// TODO make copy of all elements in result
		// TODO overlay with all elements in a
		cell_unref(result);
		cell_unref(args);
		return error_rt1("assoc & not yet implemented", a);
            }

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
    assert(args == NIL);
    return result;
}

static cell *cfun2_ref(cell *a, cell *b) {
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
	if (!assoc_get(a, b, &value)) {
	    cell_unref(a);
	    return error_rt1("assoc key does not exist", b);
	}
	cell_unref(a);
	cell_unref(b);
	return value;

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
    case c_CFUNQ:
    case c_CFUN1:
    case c_CFUN2:
    case c_CFUN3:
    // TODO ref should work for functions ??
    case c_SPECIAL:
    default:
	 break;
    }
    cell_unref(b);
    return error_rt1("cannot referrence", a);
    return 0;
}

static cell *cfunQ_refq(cell *args, environment *env) {
    cell *a, *b;
    if (arg2(args, &a, &b)) {
	a = cfun2_ref(eval(a, env), b);
    }
    return a;
}

static cell *cfun1_use(cell *a) {
    char *str;
    cell_ref(a); // for error message
    if (!get_cstring(a, &str, a)) {
        return cell_ref(hash_void); // error
    }
    if (strcmp(str, "io") == 0) {
	extern cell *module_io();
	cell_unref(a);
	return module_io();
    }
#ifdef HAVE_GTK
    if (strcmp(str, "gtk") == 0) {
        extern cell *module_gtk();
	cell_unref(a);
        return module_gtk();
    }
#endif
    return error_rt1("module not found", a); // should
}

void cfun_init() {
    // TODO hash_amp etc are unrefferenced, and depends on oblist
    //      to keep symbols in play
    hash_amp     = oblistv("#amp",     cell_cfunN(cfunN_amp));
    hash_assoc   = oblistv("#assoc",   cell_cfunQ(cfunQ_assoc));
    hash_defq    = oblistv("#defq",    cell_cfunQ(cfunQ_defq));
    hash_div     = oblistv("#div",     cell_cfun2(cfun2_div));
    hash_if      = oblistv("#if",      cell_cfunQ(cfunQ_if));
    hash_lambda  = oblistv("#lambda",  cell_cfunQ(cfunQ_lambda));
    hash_lt      = oblistv("#lt",      cell_cfunN(cfunN_lt));
    hash_list    = oblistv("#",        cell_cfunN(cfunN_list));
    hash_minus   = oblistv("#minus",   cell_cfunN(cfunN_minus));
    hash_not     = oblistv("#not",     cell_cfun1(cfun1_not));
    hash_plus    = oblistv("#plus",    cell_cfunN(cfunN_plus));
    hash_quote   = oblistv("#quote",   cell_cfunQ(cfunQ_quote));
    hash_ref     = oblistv("#ref",     cell_cfun2(cfun2_ref));
    hash_refq    = oblistv("#refq",    cell_cfunQ(cfunQ_refq));
    hash_times   = oblistv("#times",   cell_cfunN(cfunN_times));
    hash_use     = oblistv("#use",     cell_cfun1(cfun1_use));
    hash_vector  = oblistv("#vector",  cell_cfunQ(cfunQ_vector));

    // values
    hash_f       = oblistv("#f",       NIL);
    hash_t       = oblistv("#t",       NIL);
    hash_void    = oblistv("#void",    NIL);
    hash_args    = oblistv("#args",    NIL); // TODO argc, argv
    // values are themselves
    oblist_set(hash_f,    cell_ref(hash_f));
    oblist_set(hash_t,    cell_ref(hash_t));
    oblist_set(hash_void, cell_ref(hash_void));
}

void cfun_drop() {
    // loose circular definitions
    oblist_set(hash_f, NIL);
    oblist_set(hash_t, NIL);
    oblist_set(hash_void, NIL);
}

