/*  TAB-P
 *
 *  TODO should evaluation happen in functions? perhaps
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cfun.h"
#include "oblist.h"
#include "err.h"

static void cfun_exit(void);

static cell *cfunQ_defq(cell *args, cell *env) {
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
	assert(env_assoc(env));
	if (!assoc_set(env_assoc(env), a, cell_ref(b))) {
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

static cell *cfunQ_and(cell *args, cell *env) {
    int bool = 1;
    cell *a;

    while (list_split2(&args, &a)) {
	a = eval(a, env);
	if (!get_boolean(a, &bool, args)) {
	    return cell_ref(hash_void); // error
	}
	if (!bool) {
	    cell_unref(args);
	    break;
	}
    }
    return cell_ref(bool ? hash_t : hash_f);
}

static cell *cfunQ_or(cell *args, cell *env) {
    int bool = 0;
    cell *a;

    while (list_split2(&args, &a)) {
	a = eval(a, env);
	if (!get_boolean(a, &bool, args)) {
	    return cell_ref(hash_void); // error
	}
	if (bool) {
	    cell_unref(args);
	    break;
	}
    }
    return cell_ref(bool ? hash_t : hash_f);
}

static cell *cfun1_not(cell *a) {
    int bool;
    if (!get_boolean(a, &bool, NIL)) {
	return cell_ref(hash_void); // error
    }
    return bool ? cell_ref(hash_f) : cell_ref(hash_t);
}

static cell *cfunQ_if(cell *args, cell *env) {
    int bool;
    cell *a, *b;
    if (!arg2(args, &a, &b)) {
	return a; // error
    }
    a = eval(a, env);
    if (!get_boolean(a, &bool, b)) {
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
	*env_progp(env) = cell_list(a, env_prog(env));
        return NIL;
    } else {
        return eval(a, env);
    }
}

static cell *cfunQ_quote(cell *args, cell *env) {
    cell *a;
    arg1(args, &a); // sets void if error
    return a;
}

static cell *cfunQ_lambda(cell *args, cell *env) {
    cell *arglist;
    cell *cp;
    if (!list_split2(&args, &arglist)) {
        return error_rt1("Missing function argument list", args);
    }
    cp = cell_lambda(arglist, args);

    // all functions are continuations
    cp = cell_cont(cp, cell_ref(env));

    return cp;
}

static cell *cfunN_plus(cell *args) {
    integer_t result = 0;
    integer_t operand;
    cell *a;
    while (list_split2(&args, &a)) {
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
    if (!list_split2(&args, &a)
     || !get_integer(a, &result, args)) return cell_ref(hash_void); // error
    if (args == NIL) {
        // special case, one argument
        result = -result; // TODO overflow
    }
    while (list_split2(&args, &a)) {
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
    while (list_split2(&args, &a)) {
        if (!get_integer(a, &operand, args)) return cell_ref(hash_void);
        result *= operand; // TODO overflow etc
    }
    assert(args == NIL);
    return cell_integer(result);
}

// TODO div mod quotient - in standard lisp quotient is integer division
static cell *cfunN_quotient(cell *args) {
    integer_t result;
    integer_t operand;
    cell *a;
    if (!list_split2(&args, &a)) {
        return error_rt1("at least one argument required", args);
    }
    // in standard lisp (/ 5) is shorthand for 1/5
    if (!get_integer(a, &result, args)) return cell_ref(hash_void); // error

    while (list_split2(&args, &a)) {
        if (!get_integer(a, &operand, args)) return cell_ref(hash_void); // error

        if (operand == 0) {
            return error_rt0("attempted division by zero");
        }
        result /= operand;
    }
    return cell_integer(result);
}

static cell *cfunN_lt(cell *args) {
    int argno = 0;
    integer_t value;
    integer_t operand;
    cell *a;
    // TODO should be cfunQ_lt, do not evaluate more than necessary
    while (list_split2(&args, &a)) {
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

static cell *cfunQ_vector(cell *args, cell *env) {
    cell *vector;
    index_t len;
    cell *a;
    // vector of length determined by initialization
    len = 0;
    vector = cell_vector(0);
    // TODO rather inefficient
    while (list_split2(&args, &a)) {
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

static cell *cfunQ_assoc(cell *args, cell *env) {
    cell *a;
    cell *b;
    cell *assoc = cell_assoc();
    while (list_split2(&args, &a)) {
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
    while (list_split2(&args, &a)) {
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

// equals
// TODO evaluate only as far as required. Also applies to gt, and etc
static cell *cfunN_eq(cell *args) {
    cell *first;
    cell *a = NIL;
    int eq = 1;
    if (!list_split2(&args, &first)) {
        return cell_ref(hash_void); // error
    }
    while (list_split2(&args, &a)) {
        if (a != first) switch (a ? a->type : c_LIST) {

        case c_STRING:
            if (!cell_is_string(first)
             || first->_.string.len != a->_.string.len
             || memcmp(first->_.string.ptr, a->_.string.ptr, a->_.string.len) != 0) {
                eq = 0;
                cell_unref(args);
                args = NIL;
            }
            break;

        case c_INTEGER:
            if (!cell_is_integer(first)
             || first->_.ivalue != a->_.ivalue) {
                eq = 0;
                cell_unref(args);
                args = NIL;
            }
            break;

        case c_ASSOC:  // TODO not (yet) implemented
        case c_LIST:   // TODO not (yet) implemented
        case c_VECTOR: // TODO not (yet) implemented
        case c_SYMBOL: // straight comparison is enough
	case c_FUNC:
        case c_CONT:
        default:
            eq = 0;
            cell_unref(args);
            args = NIL;
            break;
        }
        a = NIL;
    }
    cell_unref(first);
    cell_unref(a);
    cell_unref(args);
    return cell_ref(eq ? hash_t : hash_f);
}

static cell *cfunN_noteq(cell *args) {
    cell *eq = cfunN_eq(args);
    cell *result = cell_ref((eq == hash_t) ? hash_f : hash_t);
    cell_unref(eq);
    return result;
}

static cell *cfun2_ref(cell *a, cell *b) {
    cell *value;
    if (cell_is_assoc(a)) {
	if (!assoc_get(a, b, &value)) {
	    cell_unref(a);
	    return error_rt1("assoc key does not exist", b);
	}
    } else {
	index_t index;
	if (!get_index(b, &index, a)) return cell_ref(hash_void); // error
	value = ref_index(a, index);
    }
    cell_unref(b);
    cell_unref(a);
    return value;
}

static cell *cfunQ_refq(cell *args, cell *env) {
    cell *a, *b;
    if (arg2(args, &a, &b)) {
	a = cfun2_ref(eval(a, env), b);
    }
    return a;
}

static cell *cfun1_use(cell *a) {
    char *str;
    cell_ref(a); // for error message
    if (!get_cstring(a, &str, NIL)) {
        return cell_ref(hash_void); // error
    }
    if (strcmp(str, "io") == 0) {
	extern cell *module_io();
	cell_unref(a);
	return module_io();
    }
#ifdef HAVE_GTK
    if (strcmp(str, "gtk3") == 0) {
        extern cell *module_gtk();
	cell_unref(a);
        return module_gtk();
    }
#endif
    return error_rt1("module not found", a); // should
}

static cell *cfun1_exit(cell *a) {
    integer_t val;
    if (!get_integer(a, &val, NIL)) { // optional...
        val = 0;
    }

    // TODO check range
    exit(val);

    assert(0); // should never reach here
    return NIL;
}

// set #args
void cfun_args(int argc, char * const argv[]) {
    cell *vector = NIL;
    assert(hash_args == NIL); // should not be invoked twice
    if (argc > 0) {
        int i;
        vector = cell_vector(argc);
        for (i = 0; i < argc; ++i) {
            // TODO can optimize by not allocating fixed string here and in cfun_init
	    char_t *s = strdup(argv[i]);
	    assert(s);
	    vector->_.vector.table[i] = cell_astring(s, strlen(s));
        }
    }
    hash_args = oblistv("#args", vector);
}

void cfun_init() {
    // TODO hash_amp etc are unrefferenced, and depends on oblist
    //      to keep symbols in play
    hash_amp      = oblistv("#amp",      cell_cfunN(cfunN_amp));
    hash_and      = oblistv("#and",      cell_cfunQ(cfunQ_and));
    hash_assoc    = oblistv("#assoc",    cell_cfunQ(cfunQ_assoc));
    hash_defq     = oblistv("#defq",     cell_cfunQ(cfunQ_defq));
    hash_quotient = oblistv("#quotient", cell_cfunN(cfunN_quotient));
    hash_eq       = oblistv("#eq",       cell_cfunN(cfunN_eq));
		    oblistv("#exit",     cell_cfunN(cfun1_exit));
    hash_if       = oblistv("#if",       cell_cfunQ(cfunQ_if));
    hash_lambda   = oblistv("#lambda",   cell_cfunQ(cfunQ_lambda));
    hash_lt       = oblistv("#lt",       cell_cfunN(cfunN_lt));
    hash_list     = oblistv("#",         cell_cfunN(cfunN_list)); // TODO
    hash_minus    = oblistv("#minus",    cell_cfunN(cfunN_minus));
    hash_not      = oblistv("#not",      cell_cfun1(cfun1_not));
    hash_noteq    = oblistv("#noteq",    cell_cfunN(cfunN_noteq));
    hash_or       = oblistv("#or",       cell_cfunQ(cfunQ_or));
    hash_plus     = oblistv("#plus",     cell_cfunN(cfunN_plus));
    hash_quote    = oblistv("#quote",    cell_cfunQ(cfunQ_quote));
    hash_ref      = oblistv("#ref",      cell_cfun2(cfun2_ref));
    hash_refq     = oblistv("#refq",     cell_cfunQ(cfunQ_refq));
    hash_times    = oblistv("#times",    cell_cfunN(cfunN_times));
		    oblistv("#use",      cell_cfun1(cfun1_use));
    hash_vector   = oblistv("#vector",   cell_cfunQ(cfunQ_vector));

    // values
    hash_f       = oblistv("#f",       NIL);
    hash_t       = oblistv("#t",       NIL);
    hash_void    = oblistv("#void",    NIL);
    hash_undefined = oblistv("#undefined", NIL); // TODO should it be visible?
    // values are themselves
    oblist_set(hash_f,         cell_ref(hash_f));
    oblist_set(hash_t,         cell_ref(hash_t));
    oblist_set(hash_void,      cell_ref(hash_void));
    oblist_set(hash_undefined, cell_ref(hash_undefined));

    atexit(cfun_exit);
}

static void cfun_exit(void) {
    // loose circular definitions
    oblist_set(hash_f, NIL);
    oblist_set(hash_t, NIL);
    oblist_set(hash_void, NIL);
    oblist_set(hash_undefined, NIL);
}

