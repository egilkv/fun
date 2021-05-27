/*  TAB-P
 *
 *  TODO should evaluation happen in functions? perhaps
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "cfun.h"
#include "oblist.h"
#include "number.h"
#include "err.h"
#include "parse.h" // chomp_file
#include "debug.h"
#if HAVE_MATH
#include "m_math.h"
#endif

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

    while (list_pop(&args, &a)) {
	a = eval(a, env);
	if (!get_boolean(a, &bool, args)) {
            return cell_void(); // error
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

    while (list_pop(&args, &a)) {
	a = eval(a, env);
	if (!get_boolean(a, &bool, args)) {
            return cell_void(); // error
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
        return cell_void(); // error
    }
    return bool ? cell_ref(hash_f) : cell_ref(hash_t);
}

static cell *cfunQ_if(cell *args, cell *env) {
    int bool;
    cell *a, *b, *c;

    if (!list_pop(&args, &a)) {
	cell_unref(args);
	return error_rt0("missing condition for if");
    }
    if (!list_pop(&args, &b)) {
	cell_unref(a);
	cell_unref(args);
	return error_rt0("missing value if true for if");
    }
    if (list_pop(&args, &c)) { // if-then-else
	arg0(args);
    } else {
	c = cell_void(); // TODO can optimize
    }
    a = eval(a, env);
    if (!get_boolean(a, &bool, args)) {
        return cell_void(); // error
    }
    if (bool) {
	cell_unref(c);
	a = b;
    } else {
	cell_unref(b);
	a = c;
    }
#if 1 // TODO enable...
    if (env) {
	// evaluate in-line
	*env_progp(env) = cell_list(a, env_prog(env));
        return NIL;
    } else 
#endif
    {
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
    if (!list_pop(&args, &arglist)) {
        return error_rt1("Missing function argument list", args);
    }
    // all functions are continuations
    cp = cell_lambda(arglist, args);
    if (env != NIL) {
        cp = cell_closure(cp, cell_ref(env));
    }
    return cp;
}

static cell *cfunN_plus(cell *args) {
    number result;
    number operand;
    cell *a;
    result.dividend.ival = 0;
    result.divisor = 1;
    while (list_pop(&args, &a)) {
        if (!get_number(a, &operand, args)) return cell_void();
	if (sync_float(&result, &operand)) {
            result.dividend.fval += operand.dividend.fval;
            if (!isfinite(result.dividend.fval)) {
		return err_overflow(args);
            }
        } else {
#ifdef __GNUC__
            if (__builtin_smulll_overflow(result.dividend.ival,
                                          operand.divisor,
                                          &(result.dividend.ival))
             || __builtin_smulll_overflow(operand.dividend.ival,
                                          result.divisor,
                                          &(operand.dividend.ival))
             || __builtin_saddll_overflow(result.dividend.ival,
                                          operand.dividend.ival,
                                          &(result.dividend.ival))
             || __builtin_smulll_overflow(result.divisor,
                                          operand.divisor,
                                          &(result.divisor))) {
		return err_overflow(args);
            }
#else
            // no overflow detection
            result.dividend.ival = result.dividend.ival * operand.divisor +
                                   operand.dividend.ival * result.divisor;
            result.divisor *= operand.divisor;
#endif
            normalize_q(&result);
        }
    }
    assert(args == NIL);
    return cell_number(&result);
}

static cell *cfunN_minus(cell *args) {
    number result;
    number operand;
    cell *a;
    result.dividend.ival = 0;
    result.divisor = 1;
    if (!list_pop(&args, &a)
     || !get_number(a, &result, args)) return cell_void(); // error
    if (args == NIL) { // special case, one argument?
        if (!make_negative(&result)) {
            return err_overflow(args);
        }
    }
    while (list_pop(&args, &a)) {
        if (!get_number(a, &operand, args)) return cell_void();
	if (sync_float(&result, &operand)) {
            result.dividend.fval -= operand.dividend.fval;
            if (!isfinite(result.dividend.fval)) {
		return err_overflow(args);
            }
        } else {
#ifdef __GNUC__
            if (__builtin_smulll_overflow(result.dividend.ival,
                                          operand.divisor,
                                          &(result.dividend.ival))
             || __builtin_smulll_overflow(operand.dividend.ival,
                                          result.divisor,
                                          &(operand.dividend.ival))
             || __builtin_ssubll_overflow(result.dividend.ival,
                                          operand.dividend.ival,
                                          &(result.dividend.ival))
             || __builtin_smulll_overflow(result.divisor,
                                          operand.divisor,
                                          &(result.divisor))) {
		return err_overflow(args);
            }
#else
            // no overflow detection
            result.dividend.ival = result.dividend.ival * operand.divisor -
                                   operand.dividend.ival * result.divisor;
            result.divisor *= operand.divisor;
#endif
            normalize_q(&result);
        }
    }
    assert(args == NIL);
    return cell_number(&result);
}

static cell *cfunN_times(cell *args) {
    number result;
    number operand;
    cell *a;
    result.dividend.ival = 1;
    result.divisor = 1;
    while (list_pop(&args, &a)) {
        if (!get_number(a, &operand, args)) return cell_void();
	if (sync_float(&result, &operand)) {
            result.dividend.fval *= operand.dividend.fval;
            if (!isfinite(result.dividend.fval)) {
		return err_overflow(args);
            }
        } else {
#ifdef __GNUC__
            if (__builtin_smulll_overflow(result.dividend.ival,
                                          operand.dividend.ival,
                                          &(result.dividend.ival))) {
		return err_overflow(args);
            }
#else
            // no overflow detection
            result.divisor *= operand.divisor;
#endif
            normalize_q(&result);
        }
        // TODO overflow etc
    }
    assert(args == NIL);
    return cell_number(&result);
}

// TODO div mod quotient - in standard lisp quotient is integer division
static cell *cfunN_quotient(cell *args) {
    number result;
    number operand;
    cell *a;
    if (!list_pop(&args, &a)) {
        return error_rt1("at least one argument required", args);
    }
    // remark: in standard lisp (/ 5) is shorthand for 1/5
    if (!get_number(a, &result, args)) return cell_void(); // error

    while (list_pop(&args, &a)) {
        if (!get_number(a, &operand, args)) return cell_void(); // error
	if (sync_float(&result, &operand)) {
	    result.dividend.fval /= operand.dividend.fval;
            if (!isfinite(result.dividend.fval)) {
                if (operand.dividend.fval == 0.0) {
                    cell_unref(args);
                    return error_rt0("attempted division by zero");
                } else {
                    return err_overflow(args);
                }
            }
	} else {
#ifdef __GNUC__
            if (__builtin_smulll_overflow(result.dividend.ival,
                                          operand.divisor,
                                          &(result.dividend.ival))
             || __builtin_smulll_overflow(result.divisor,
                                          operand.dividend.ival,
                                          &(result.divisor))) {
		return err_overflow(args);
            }
#else
            // no overflow detection
            result.dividend.ival *= operand.divisor;
            result.divisor *= operand.dividend.ival;
#endif
	    if (result.divisor == 0) {
                cell_unref(args);
		return error_rt0("attempted division by zero");
	    }
            normalize_q(&result);
	}
    }
    return cell_number(&result);
}

//
//  compare function as macro, which will be expanded four times
//  TODO consider common function with icompare and fcompare
//       function passed as parameter instead
//
#define CFUNN_COMPARE(funname, OP)                                  \
static cell *funname(cell *args) {                                  \
    cell *a;                                                        \
    /* TODO could be cfunQ, do not evaluate more than necessary */  \
    if (!list_pop(&args, &a)) return cell_void();                   \
    if (cell_is_string(a)) {                                        \
        /* compare strings */                                       \
        cell *value = a;                                            \
        char_t *v_ptr;                                              \
        index_t v_len;                                              \
        if (!peek_string(a, &v_ptr, &v_len, NIL)) {                 \
            assert(0);                                              \
        }                                                           \
        while (list_pop(&args, &a)) {                               \
            char_t *ptr;                                            \
            index_t len;                                            \
            int cmp;                                                \
            if (!peek_string(a, &ptr, &len, args)) {                \
                cell_unref(value);                                  \
                return cell_void(); /* error */                     \
            }                                                       \
            /* TODO compare using length instead */                 \
            cmp = strcmp(v_ptr, ptr);                               \
            if (cmp OP 0) {                                         \
                cell_unref(value);                                  \
                value = a;                                          \
                v_ptr = ptr;                                        \
                v_len = len;                                        \
            } else {                                                \
                cell_unref(value);                                  \
                cell_unref(a);                                      \
                cell_unref(args);                                   \
                return cell_ref(hash_f); /* false */                \
            }                                                       \
        }                                                           \
        cell_unref(value);                                          \
    } else {                                                        \
        /* compare numbers */                                       \
        number value;                                               \
        number operand;                                             \
        if (!get_number(a, &value, args)) {                         \
            return cell_void(); /* error */                         \
        }                                                           \
        while (list_pop(&args, &a)) {                               \
            if (!get_number(a, &operand, args)) {                   \
                return cell_void(); /* error */                     \
            }                                                       \
            if (value.divisor == 1 && operand.divisor == 1) {       \
                if (value.dividend.ival OP operand.dividend.ival) { \
                    value.dividend.ival = operand.dividend.ival;    \
                } else {                                            \
                    cell_unref(args);                               \
                    return cell_ref(hash_f); /* false */            \
                }                                                   \
            } else {                                                \
                /* TODO should do quotients smarter */              \
                make_float(&value);                                 \
                make_float(&operand);                               \
                if (value.dividend.fval OP operand.dividend.fval) { \
                    value.dividend.fval = operand.dividend.fval;    \
                } else {                                            \
                    cell_unref(args);                               \
                    return cell_ref(hash_f); /* false */            \
                }                                                   \
            }                                                       \
        }                                                           \
    }                                                               \
    assert(args == NIL);                                            \
    return cell_ref(hash_t);                                        \
}

#define C_GE >=
#define C_GT >
#define C_LE <=
#define C_LT <

CFUNN_COMPARE(cfunN_ge, C_GE)
CFUNN_COMPARE(cfunN_gt, C_GT)
CFUNN_COMPARE(cfunN_le, C_LE)
CFUNN_COMPARE(cfunN_lt, C_LT)

static cell *cfunN_list(cell *args) {
    return args;
}

static cell *cfunN_vector(cell *args) {
    cell *vector = NIL; // empty vector is NIL
    index_t len;
    cell *a;
    // vector of length determined by initialization
    len = 0;
    // TODO rather inefficient
    while (list_pop(&args, &a)) {
        if (cell_is_label(a)) { // index : value
	    index_t index;
            cell *b;
            label_split(a, &a, &b);
            if (get_index(a, &index, b)) { // TODO a not evaluated?
                if (index >= len) {
                    len = index+1;
                    if (vector == NIL) vector = cell_vector(len);
                    else vector_resize(vector, len);
                }
                // TODO check if redefining, which is not allowed
                if (!vector_set(vector, index, b)) {
                    assert(0); // out of bounds should not happen
                }
            }
        } else {
            ++len;
            if (vector == NIL) vector = cell_vector(len);
            else vector_resize(vector, len);
            if (!vector_set(vector, len-1, a)) {
                assert(0); // out of bounds should not happen
            }
        }
    }
    assert(args == NIL);
    return vector;
}

static cell *cfunN_assoc(cell *args) {
    cell *a;
    cell *b;
    cell *assoc = cell_assoc();
    while (list_pop(&args, &a)) {
        if (!cell_is_label(a)) {
	    cell_unref(error_rt1("initialization item not in form of key: value", a));
        } else {
	    label_split(a, &a, &b);
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
    while (list_pop(&args, &a)) {
	switch (a ? a->type : c_LIST) {

        case c_STRING:
            if (result == NIL) {
                result = a; // somewhat cheeky, initial condition
                // TODO consider raising alarm
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
            if (result == NIL) {
                result = a; // simple enough
            } else if (!cell_is_vector(result)) {
                // TODO vector & list should be supported
                cell_unref(result);
                cell_unref(args);
                return error_rt1("& applied to non-vector and vector", a);
            } else {
                // TODO optimize for case of more than two arguments
                index_t rlen = result->_.vector.len;
                index_t alen = a->_.vector.len;
		if (alen == 0) {
		    cell_unref(a);
		} else if (rlen == 0) {
		    cell_unref(result);
		    result = a;
		} else {
                    cell *newvector = cell_vector(rlen + alen);
                    integer_t i;
                    for (i = 0; i < rlen; ++i) {
                        newvector->_.vector.table[i] = cell_ref(result->_.vector.table[i]);
                    }
                    for (i = 0; i < alen; ++i) {
                        newvector->_.vector.table[rlen+i] = cell_ref(a->_.vector.table[i]);
                    }
		    cell_unref(result);
		    cell_unref(a);
                    result = newvector;
		}
            }
            break;

        case c_LIST: // a can be NIL
            // TODO definitely optmize for more than two
            if (a == NIL) {
                // add nothing
                // TODO but not if result is string
            } else if (result && !cell_is_list(result)) {
                // TODO vector & list should be supported
                cell_unref(result);
                cell_unref(args);
                return error_rt1("& applied to non-list and list", a);
            } else {
                cell *newlist = NIL;
                cell **pp = &newlist;
                // copy first list
                while (result) {
                    assert(cell_is_list(result));
                    *pp = cell_list(cell_ref(cell_car(result)), NIL);
                    pp = &((*pp)->_.cons.cdr);
                    result = cell_cdr(result);
                }
                *pp = a;
                cell_unref(result);
                result = newlist;
            }
            break;

        case c_ASSOC:
            if (result == NIL) {
                result = a; // somewhat cheeky, initial condition
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
    if (!list_pop(&args, &first)) {
        return cell_void(); // error
    }
    while (list_pop(&args, &a)) {
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

        case c_NUMBER:
            if (!cell_is_number(first)
             || first->_.n.dividend.ival != a->_.n.dividend.ival // 64bit
             || first->_.n.divisor != a->_.n.divisor) { // TODO compare float to integer etc
                eq = 0;
                cell_unref(args);
                args = NIL;
            }
            break;

        case c_ASSOC:  // TODO not (yet) implemented
        case c_LIST:   // TODO not (yet) implemented
        case c_VECTOR: // TODO not (yet) implemented
        case c_SYMBOL: // straight comparison is enough
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

static cell *cfun1_length(cell *a) {
#if 0
    if (cell_is_assoc(a)) {
        // TODO length of assoc may be a thing
        return error_rt1("no length for assoc", a);
    }
#endif
    integer_t length = ref_length(a);
    if (length < 0) {
        return error_rt1("no length of", cell_ref(a));
    }
    cell_unref(a);
    return cell_integer(length);
}

static cell *cfun2_ref(cell *a, cell *b) {
    cell *value;
    if (cell_is_assoc(a)) {
	if (!assoc_get(a, b, &value)) {
	    cell_unref(a);
	    return error_rt1("assoc key does not exist", b);
	}
        cell_unref(b);
    } else if (cell_is_range(b)) {
        index_t index1 = 0;
        cell *b1, *b2;
        range_split(b, &b1, &b2);
        if (b1) {
            if (!get_index(b1, &index1, a)) {
                cell_unref(b2);
                return cell_void(); // error
            }
        }
        if (b2) {
            index_t index2 = 0;
            if (!get_index(b2, &index2, a)) return cell_void(); // error
            if (index2 < index1) {
                cell_unref(a);
                return error_rti("index range cannot be reverse", index2);
            }
            value = ref_range2(a, index1, 1+index2 - index1);
        } else {
            value = ref_range1(a, index1);
        }
    } else {
	index_t index;
        if (!get_index(b, &index, a)) return cell_void(); // error
	value = ref_index(a, index);
    }
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
    if (!peek_cstring(a, &str, NIL)) {
        return cell_void(); // error
    }
#ifdef HAVE_GTK
    if (strcmp(str, "gtk3") == 0) {
        extern cell *module_gtk();
	cell_unref(a);
        return module_gtk();
    }
#endif
    if (strcmp(str, "io") == 0) {
	extern cell *module_io();
	cell_unref(a);
	return module_io();
    }
#if HAVE_MATH
    if (strcmp(str, "math") == 0) {
        extern cell *module_math();
	cell_unref(a);
        return module_math();
    }
#endif
    if (strcmp(str, "string") == 0) {
        extern cell *module_string();
	cell_unref(a);
        return module_string();
    }
    return error_rt1("module not found", a); // should
}

static cell *cfun1_exit(cell *a) {
    integer_t val;
    if (!get_integer(a, &val, NIL)) { // TODO optional...
        val = 0;
    }

    // TODO check range
    exit(val);

    assert(0); // should never reach here
    return NIL;
}

static cell *cfun1_include(cell *a) {
    char_t *name;
    index_t len;
    if (!peek_string(a, &name, &len, a)) return cell_void(); // error
    if (!chomp_file(name)) {
        return error_rt1("cannot read include-file", a);
    }
    return a;
}

// debugging, trace value being passed
static cell *cfun1_trace(cell *a) {
    debug_trace(a);
    return a;
}

// debugging, enable trace, return first (valid) argument
static cell *cfunQ_traceon(cell *args, cell *env) {
    cell *result = cell_ref(hash_void);
    cell *arg;
    while (list_pop(&args, &arg)) {
        if (!debug_traceon(arg)) {
            arg = error_rt1("cannot enable trace for", arg);
        }
        if (result == hash_void) {
            cell_unref(result);
            result = arg;
        } else {
            cell_unref(arg);
        }
    }
    return result;
}

// debugging, disable trace, return first (valid) argument
static cell *cfunQ_traceoff(cell *args, cell *env) {
    cell *result = cell_ref(hash_void);
    cell *arg;
    while (list_pop(&args, &arg)) {
        if (!debug_traceoff(arg)) {
            arg = error_rt1("cannot disable trace for", arg);
        }
        if (result == hash_void) {
            cell_unref(result);
            result = arg;
        } else {
            cell_unref(arg);
        }
    }
    return result;
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
    hash_assoc    = oblistv("#assoc",    cell_cfunN(cfunN_assoc));
                    oblistv("#trace",    cell_cfun1(cfun1_trace)); // debugging
                    oblistv("#traceoff", cell_cfunQ(cfunQ_traceoff)); // debugging
                    oblistv("#traceon",  cell_cfunQ(cfunQ_traceon)); // debugging
    hash_defq     = oblistv("#defq",     cell_cfunQ(cfunQ_defq));
    hash_quotient = oblistv("#quotient", cell_cfunN(cfunN_quotient));
    hash_eq       = oblistv("#eq",       cell_cfunN(cfunN_eq));
		    oblistv("#exit",     cell_cfunN(cfun1_exit));
    hash_ge       = oblistv("#ge",       cell_cfunN(cfunN_ge));
    hash_gt       = oblistv("#gt",       cell_cfunN(cfunN_gt));
    hash_if       = oblistv("#if",       cell_cfunQ(cfunQ_if));
                    oblistv("#include",  cell_cfun1(cfun1_include));
    hash_lambda   = oblistv("#lambda",   cell_cfunQ(cfunQ_lambda));
                    oblistv("#length",   cell_cfun1(cfun1_length));
    hash_le       = oblistv("#le",       cell_cfunN(cfunN_le));
    hash_lt       = oblistv("#lt",       cell_cfunN(cfunN_lt));
    hash_list     = oblistv("#",         cell_cfunN(cfunN_list)); // TODO remove?
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
    hash_vector   = oblistv("#vector",   cell_cfunN(cfunN_vector));

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

