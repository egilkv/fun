/*  TAB-P
 *
 *
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
#include "run.h"
#include "compile.h"
#if HAVE_MATH
#include "m_math.h"
#endif
#if HAVE_GTK
#include "m_gtk.h"
#endif
#include "m_io.h"
#include "m_bit.h"
#include "m_string.h"
#include "m_time.h"

static void cfun_exit(void);

static cell *cfun1_not(cell *a) {
    int bool;
    if (!get_boolean(a, &bool, NIL)) {
        return cell_error();
    }
    return cell_boolean(!bool);
}

static cell *cfunN_plus(cell *args) {
    number result;
    number operand;
    cell *a;
    result.dividend.ival = 0;
    result.divisor = 1;
    if (list_pop(&args, &a)) {
        if (!get_number(a, &result, args)) return cell_void();
    }
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
                                          &(result.divisor))
             || !normalize_q(&result)) {
		return err_overflow(args);
            }
#else
            // no overflow detection
            result.dividend.ival = result.dividend.ival * operand.divisor +
                                   operand.dividend.ival * result.divisor;
            result.divisor *= operand.divisor;
            normalize_q(&result);
#endif
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
    if (!at_least_one(&args, &a)
     || !get_number(a, &result, args)) return cell_error();
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
                                          &(result.divisor))
             || !normalize_q(&result)) {
		return err_overflow(args);
            }
#else
            // no overflow detection
            result.dividend.ival = result.dividend.ival * operand.divisor -
                                   operand.dividend.ival * result.divisor;
            result.divisor *= operand.divisor;
            normalize_q(&result);
#endif
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
    if (list_pop(&args, &a)) {
        if (!get_number(a, &result, args)) return cell_void();
    }
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
                                          &(result.dividend.ival))
             || __builtin_smulll_overflow(result.divisor,
                                          operand.divisor,
                                          &(result.divisor))
             || !normalize_q(&result)) {
		return err_overflow(args);
            }
#else
            // no overflow detection
            result.dividend.ival *= operand.dividend.ival;
            result.divisor *= operand.divisor;
            normalize_q(&result);
#endif
        }
    }
    assert(args == NIL);
    return cell_number(&result);
}

// TODO div mod quotient - in standard lisp quotient is integer division
static cell *cfunN_quotient(cell *args) {
    number result;
    number operand;
    cell *a;
    if (!at_least_one(&args, &a)) return cell_error();
    // remark: in standard lisp (/ 5) is shorthand for 1/5
    if (!get_number(a, &result, args)) return cell_error();

    while (list_pop(&args, &a)) {
        if (!get_number(a, &operand, args)) return cell_error();
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
            if (!normalize_q(&result)) {
		return err_overflow(args);
            }
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
    /* TODO could do lazy evaluation */                             \
    if (!at_least_one(&args, &a)) return cell_void();               \
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

// this is really vector-or-list
static cell *cfunN_list(cell *args) {
    cell *vector = NIL; // empty vector is NIL
    index_t len;
    cell *a;
    // vector of length determined by initialization
    len = 0;
    // TODO rather inefficient
    while (list_pop(&args, &a)) {
        if (cell_is_label(a)) { // index:value or index..index:value
            index_t index = len; // for range, start defaults to current position
            index_t indexN = 0;
            cell *val;
            label_split(a, &a, &val);
            if (cell_is_range(a)) {
                index_t index2;
                cell *a1, *a2;
                range_split(a, &a1, &a2);
                if (a1 == NIL || get_index(a1, &index, a2)) {
                    if (!a2) {
                        cell_unref(error_rt0("upper range required for vector index"));
                    } else if (get_index(a2, &index2, NIL)) {
                        if (index2 < index) {
                            return error_rti("index range cannot be reverse", index2);
                        } else {
                            indexN = 1+index2 - index;
                        }
                    }
                }
            } else if (get_index(a, &index, NIL)) {
                indexN = 1;
            }
            if (indexN > 0) {
                if (index+indexN > len) {
                    len = index+indexN;
                    vector = vector_resize(vector, len);
                }
                while (indexN-- > 0) {
                    if (!vector_set(vector, index, cell_ref(val))) {
                        cell_unref(error_rti("cannot redefine vector, index ", index));
                        // TODO should probably only have one error message
                    }
                    ++index;
                }
            }
            cell_unref(val);

        } else if (vector == NIL) {
            // first item is not index:value
            // TODO look for more labels?
            return cell_list(a, args); // we are dealing with a humble list

        } else {
            // vector non-indexed item
            ++len;
            vector = vector_resize(vector, len);
            if (!vector_set(vector, len-1, a)) {
                cell_unref(error_rti("cannot redefine vector, index ", len-1));
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
            if (!cell_is_symbol(a)) {
                cell_unref(error_rt1("key must be a symbol", a));
		cell_unref(b);
            } else if (!assoc_set(assoc, a, b)) {
		cell_unref(error_rt1("duplicate key ignored", a));
		cell_unref(b);
	    }
	}
    }
    assert(args == NIL);
    return assoc;
}

// b can be a vector or a list
static cell *cat_vectors(cell *a, cell *b)
{
    // TODO optimize for case of more than two arguments
    index_t alen = ref_length(a);
    index_t blen = ref_length(b);
    if (blen == 0) {
        cell_unref(b);
    } else if (alen == 0) {
        cell_unref(a);
        a = b;
    } else {
        cell *newvector = cell_vector_nil(alen + blen);
        integer_t i;
        for (i = 0; i < alen; ++i) {
            newvector->_.vector.table[i] = cell_ref(a->_.vector.table[i]);
        }
        if (cell_is_list(b)) {
            cell *b1;
            i = 0;
            while (list_pop(&b, &b1)) {
                newvector->_.vector.table[alen+i++] = b1;
            }
            assert(i == blen);
        } else {
            for (i = 0; i < blen; ++i) {
                newvector->_.vector.table[alen+i] = cell_ref(b->_.vector.table[i]);
            }
        }
        cell_unref(a);
        cell_unref(b);
        a = newvector;
    }
    return a;
}

// add list to a list
static void add_list(cell *addlist, cell ***ppp) {
    cell *p = addlist;
    cell **pp = *ppp;
    while (p) {
        *pp = cell_list(cell_ref(cell_car(p)), NIL);
        pp = &((*pp)->_.cons.cdr);
        p = cell_cdr(p);
    }
    *ppp = pp;
    cell_unref(addlist);
}

// catenate lists, vectors or strings
static cell *cfunN_cat(cell *args) {
    cell *a;
    cell *result = NIL;

    while (list_pop(&args, &a)) {
	switch (a ? a->type : c_LIST) {

        case c_STRING:
            if (result == NIL) {
                result = a; // TODO somewhat cheeky, initial condition
            } else if (!cell_is_string(result)) {
                cell_unref(result);
                cell_unref(args);
                return error_rt1("cannon catenate non-string and string", a);
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
            } else switch (result->type) {
            case c_VECTOR:
                // TODO optimize for case of more than two arguments
                result = cat_vectors(result, a);
                break;
            case c_LIST:
                {
                    // TODO optimize for case of more than two arguments
                    index_t i;
                    cell *newlist = NIL;
                    cell **pp = &newlist;
                    add_list(result, &pp); // copy first list
                    result = newlist;
                    // then append elements from the vector
                    for (i=0; i < a->_.vector.len; ++i) {
                        *pp = cell_list(cell_ref(a->_.vector.table[i]), NIL);
                        pp = &((*pp)->_.cons.cdr);
                    }
                    cell_unref(a);
                }
                break;
            default:
                cell_unref(result);
                cell_unref(args);
                return error_rt1("cannon catenate non-list and list", a);
            }
            break;

        case c_LIST: // a can be NIL
            // TODO definitely optmize for more than two
            if (a == NIL) {
                // add nothing
                // TODO but not if result is string
            } else if (result == NIL) {
                result = a; // simple enough
            } else switch (result->type) {
            case c_VECTOR:
                result = cat_vectors(result, a);
                break;
            case c_LIST:
                {
                    // TODO optimize for case of more than two arguments
                    cell *newlist = NIL;
                    cell **pp = &newlist;
                    add_list(result, &pp); // copy first list
                    result = newlist;
                    *pp = a; // then connect second list
                }
                break;
            default:
                cell_unref(result);
                cell_unref(args);
                return error_rt1("cannon catenate non-list and list", a);
            }
            break;

        case c_ASSOC:
            if (result == NIL) {
                result = a; // somewhat cheeky, initial condition
            } else if (cell_is_assoc(result)) {
                cell *newassoc = cell_assoc();
                struct assoc_i iter;
                cell *p;

                // make a copy of first assoc
                assoc_iter(&iter, result);
                while ((p = assoc_next(&iter))) {
                    if (!assoc_set(newassoc, cell_ref(assoc_key(p)), cell_ref(assoc_val(p)))) {
                        // already defined, should not happen
                        assert(0);
                    }
                }

                // add contents of second assoc
                assoc_iter(&iter, a);
                while ((p = assoc_next(&iter))) {
                    if (!assoc_set(newassoc, cell_ref(assoc_key(p)), cell_ref(assoc_val(p)))) {
                        // already defined
                        cell_unref(error_rt1("duplicate key ignored", assoc_key(p)));
                        cell_unref(assoc_val(p));
                    }
                }
		cell_unref(result);
                cell_unref(a);
                result = newassoc;
            } else {
                cell_unref(result);
                cell_unref(args);
                return error_rt1("cannon catenate non-assoc and assoc", a);
            }
            break;

        default:
            cell_unref(result);
            cell_unref(args);
            return error_rt1("cannot catenate", a);
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
    if (!at_least_one(&args, &first)) return cell_error();

    if (first == hash_undef || first == hash_void) {
        cell_unref(args);
        return error_rt1("cannot compare undefined", first);
    }

    while (list_pop(&args, &a)) {
        if (a == hash_undef || a == hash_void) {
            cell_unref(first);
            cell_unref(args);
            return error_rt1("cannot compare undefined", a);
        }
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
        cell_unref(a);
        a = NIL;
    }
    cell_unref(first);
    cell_unref(args);
    return cell_boolean(eq);
}

static cell *cfunN_noteq(cell *args) {
    int bool;
    cell *eq = cfunN_eq(args);
    if (peek_boolean(eq, &bool)) {
        cell_unref(eq);
        eq = cell_boolean(!bool);
    }
    return eq;
}

static cell *cfun1_count(cell *a) {
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

static cell *cfun1_use(cell *a) {
    char *str;
    if (!peek_cstring(a, &str, NIL)) {
        return cell_error();
    }
#ifdef HAVE_GTK
    if (strcmp(str, "gtk3") == 0) {
	cell_unref(a);
        return module_gtk();
    }
#endif
    if (strcmp(str, "io") == 0) {
	cell_unref(a);
	return module_io();
    }
#if HAVE_MATH
    if (strcmp(str, "math") == 0) {
	cell_unref(a);
        return module_math();
    }
#endif
    if (strcmp(str, "string") == 0) {
	cell_unref(a);
        return module_string();
    }
    if (strcmp(str, "bit") == 0) {
	cell_unref(a);
        return module_bit();
    }
    if (strcmp(str, "time") == 0) {
	cell_unref(a);
        return module_time();
    }
    return error_rt1("module not found", a); // should
}

static cell *cfun1_type(cell *a) {
    const char *t = NULL;
    switch (a ? a->type : c_LIST) {
    case c_LIST:
        t = "list";
        break;

    case c_VECTOR:
        t = "vector";
        break;

    case c_ASSOC:
        t = "assoc"; // TODO association list
        break;

    case c_RANGE:
        t = "range"; // TODO so this is a type
        break;

    case c_NUMBER:
        switch (a->_.n.divisor) {
        case 1:
            t = "integer";
            break;
        case 0:
            t = "real";
            break;
        default:
            t = "ratio"; // TODO name?
            break;
        }
        break;

    case c_STRING:
        t = "string";
        break;

    case c_SYMBOL:
        if (a == hash_t || a == hash_f) {
            t = "boolean";
        } else if (a == hash_void) {
            t = "void";
        } else if (a == hash_undef) {
            t = "undefined";
        } else {
            t = "symbol";
        }
        break;

    case c_LABEL:
        t = "label";
        break;

    case c_CHANNEL:
    case c_RCHANNEL:
        t = "channel";
        break;

    case c_SPECIAL:
        t = (*(a->_.special.magicf))(NULL);
        break;

    case c_CLOSURE:
    case c_CLOSURE0: // TODO are closures at global level really closures?
    case c_CLOSURE0T:
        t = "closure";
        break;

    case c_FUNC:
    case c_CFUN1:
    case c_CFUN2:
    case c_CFUNN:
    case c_CFUNR:
        t = "function"; // happens only for built in functions
        break;

    case c_ELIST: // internal use only
    case c_PAIR:  // cannot be seen in the flesh
    case c_ENV:
    case c_STOP:
    case c_KEYVAL:
    case c_DOQPUSH:
    case c_DOLPUSH:
    case c_DOGPUSH:
    case c_DOCALL1:
    case c_DOCALL2:
    case c_DOCALLN:
    case c_DOCOND:
    case c_DODEFQ:
    case c_DOREFQ:
    case c_DOLAMB:
    case c_DOLABEL:
    case c_DORANGE:
    case c_DOAPPLY:
    case c_DOPOP:
    case c_DONOOP:
        t = "internal"; // TODO should not happen
        break;
    case c_FREE:
        assert(0);
    }
    cell_unref(a);
    // TODO can optimize by storing symbols
    assert(t);
    return cell_symbol(t);
}

static cell *cfunN_exit(cell *args) {
    integer_t sts = 0;
    cell *arg;
    if (list_pop(&args, &arg)) { // one optional argument
        if (!get_integer(arg, &sts, NIL)) { // special for exit, skip errors
            sts = 0; // TODO is this a good exit status on error?
        }
    }
    arg0(args);

    // TODO check range
    exit(sts);

    assert(0); // should never reach here
    return NIL;
}

static cell *cfunN_error(cell *args) {
    // TODO improve...
    return error_rt1("error", args);
}

// return last (or only) item in file
static cell *cfun1_include(cell *a) {
    char_t *name;
    index_t len;
    cell *result = NIL;
    if (!peek_string(a, &name, &len, a)) return cell_error();
    if (!chomp_file(name, &result)) {
        return error_rt1("cannot find include-file", a);
    }
    cell_unref(a);
    return result;
}

// start coroutine
static cell *cfun1_go(cell *args) {
    cell *thunk;
    cell *prog;
    if (!at_least_one(&args, &thunk)) return cell_error();

    prog = known_thunk_a(thunk, args);
    start_process(cell_ref(prog), NIL /*env*/, NIL /*stack*/);

    return cell_void();
}

// start coroutine
static cell *cfunN_channel(cell *args) {
    arg0(args);
    return cell_channel();
}

// receive from channel
static void cfunR_receive(cell *args) {
    cell *chan;
    struct proc_run_env *pre;
    if (!arg1(args, &chan)) {
        push_stack_current_run_env(cell_error());
        return;
    }
    switch (chan ? chan->type : c_LIST) {
    case c_CHANNEL:
        if ((pre = chan->_.channel.senders)) {
            cell *result;
            if (!list_pop(&(pre->stack), &result)) { // writer waiting: pick up argument from its stack
                assert(0); // TODO can this happen?
            }
            push_stack_current_run_env(result); // receive result to our own stack

            pre->stack = cell_list(cell_void(), pre->stack); // push back send's result on other stack
            chan->_.channel.senders = pre->next; // move to top of ready list
            pre->next = NULL;
            prepend_proc_list(&ready_list, pre);
        } else {
            append_proc_list(&(chan->_.channel.receivers), suspend());
            // no result on stack
        }
        break;

    case c_RCHANNEL:
        if (chan->_.rchannel.buffer) { // data ready?
            cell *result;
            if (!list_pop(&(chan->_.rchannel.buffer), &result)) {
                assert(0);
            }
            push_stack_current_run_env(result); // receive result to our own stack
        } else {
            append_proc_list(&(chan->_.rchannel.receivers), suspend());
            // no result on stack
        }
        break;

    default:
        push_stack_current_run_env(error_rt1("not a channel", chan));
        return;
    }
    cell_unref(chan);
}

// send to channel
static void cfunR_send(cell *args) {
    cell *chan;
    cell *val;
    struct proc_run_env *pre;
    if (!arg2(args, &chan, &val)) {
        push_stack_current_run_env(cell_error());
        return;
    }
    switch (chan ? chan->type : c_LIST) {
    case c_CHANNEL:
        if ((pre = chan->_.channel.receivers)) {
            pre->stack = cell_list(val, pre->stack); // reader waiting: push read result directly on its stack
            chan->_.channel.receivers = pre->next; // and move it to top of ready list
            pre->next = NULL;
            prepend_proc_list(&ready_list, pre);
            push_stack_current_run_env(cell_void()); // return regular result of send
        } else {
            // no reader waiting, push the argument on our own stack, then suspend
            push_stack_current_run_env(val);
            append_proc_list(&(chan->_.channel.senders), suspend());
            // no result on stack
        }
        break;

    case c_RCHANNEL:
        if ((pre = chan->_.rchannel.receivers)) {
            pre->stack = cell_list(val, pre->stack); // reader waiting: push read result directly on its stack
            chan->_.channel.receivers = pre->next; // and move it to top of ready list
            pre->next = NULL;
            prepend_proc_list(&ready_list, pre);
            push_stack_current_run_env(cell_void()); // return regular result of send
        } else {
            // no reader waiting, push the argument on our own stack, then suspend
            push_stack_current_run_env(val);
            append_proc_list(&(chan->_.channel.senders), suspend());
            // no result on stack
        }
        break;

    default:
        cell_unref(val);
        push_stack_current_run_env(error_rt1("not a channel", chan));
        return;
    }
    cell_unref(chan);
}

// get from OS environment
static cell *cfun1_getenv(cell *a) {
    extern char **environ;
    char **pp = environ;
    char const *lookfor = NULL;

    switch (a ? a->type : c_LIST) {
    case c_STRING:
        lookfor = a->_.string.ptr;
        break;
    case c_SYMBOL:
        lookfor = a->_.symbol.nam;
        break;
    default:
        return error_rt1("neither symbol nor string", a);
    }
    if (lookfor) {
        int len = strlen(lookfor);
        while (*pp) {
            if (memcmp(lookfor, *pp, len) == 0) {
                char const *s = *pp + len;
                if (memcmp(s, "\tDEFAULT", 8) == 0) s += 8; // TODO: sure?
                if (*s == '=') {
                    s = strdup(s+1);
                    cell_unref(a);
                    return cell_astring((char_t *)s, strlen(s));
                }
            }
            ++pp;
        }
    }
    cell_unref(a);
    return cell_void();
}

// set #args
// invoked from main()
void cfun_args(int argc, char * const argv[]) {
    cell *vector = NIL;
    assert(hash_args == NIL); // should not be invoked twice
    if (argc > 0) {
        int i;
        vector = cell_vector_nil(argc);
        for (i = 0; i < argc; ++i) {
            // TODO can optimize by not allocating fixed string here and in cfun_init
            vector->_.vector.table[i] = cell_cstring(argv[i]);
        }
    }
    hash_args = symbol_set("#args", vector);
}

void cfun_init() {

    // these values are themselves
    hash_undef   = symbol_self("#undefined");
    hash_f       = symbol_self("#f");
    hash_t       = symbol_self("#t");
    hash_void    = symbol_self("#void");

    // TODO remove...
    hash_ellip   = symbol_set("...",   cell_ref(hash_void));

    atexit(cfun_exit);

    // TODO hash_and etc are unrefferenced, and depends on oblist
    // TODO #include and #exit is not pure, possibly also #use
    //      to keep symbols in play
    hash_assoc    = symbol_set("#assoc",    cell_cfunN_pure(cfunN_assoc));
    hash_cat      = symbol_set("#cat",      cell_cfunN_pure(cfunN_cat));
    hash_channel  = symbol_set("#channel",  cell_cfunN_pure(cfunN_channel));
                    symbol_set("#count",    cell_cfun1_pure(cfun1_count));
    hash_eq       = symbol_set("#eq",       cell_cfunN_pure(cfunN_eq));
                    symbol_set("#error",    cell_cfunN_pure(cfunN_error));
                    symbol_set("#exit",     cell_cfunN_pure(cfunN_exit));
                    symbol_set("#getenv",   cell_cfun1_pure(cfun1_getenv));
    hash_ge       = symbol_set("#ge",       cell_cfunN_pure(cfunN_ge));
    hash_go       = symbol_set("#go",       cell_cfunN(cfun1_go)); // TODO pure?
    hash_gt       = symbol_set("#gt",       cell_cfunN_pure(cfunN_gt));
                    symbol_set("#include",  cell_cfun1(cfun1_include)); // TODO pure?
    hash_le       = symbol_set("#le",       cell_cfunN_pure(cfunN_le));
    hash_lt       = symbol_set("#lt",       cell_cfunN_pure(cfunN_lt));
    hash_list     = symbol_set("#list",     cell_cfunN_pure(cfunN_list));
    hash_minus    = symbol_set("#minus",    cell_cfunN_pure(cfunN_minus));
    hash_not      = symbol_set("#not",      cell_cfun1_pure(cfun1_not));
    hash_noteq    = symbol_set("#noteq",    cell_cfunN_pure(cfunN_noteq));
    hash_plus     = symbol_set("#plus",     cell_cfunN_pure(cfunN_plus));
    hash_quotient = symbol_set("#quotient", cell_cfunN_pure(cfunN_quotient));
    hash_receive  = symbol_set("#receive",  cell_cfunR(cfunR_receive));
    hash_ref      = symbol_set("#ref",      cell_cfun2_pure(cfun2_ref));
    hash_send     = symbol_set("#send",     cell_cfunR(cfunR_send));
    hash_times    = symbol_set("#times",    cell_cfunN_pure(cfunN_times));
                    symbol_set("#type",     cell_cfun1_pure(cfun1_type));
                    symbol_set("#use",      cell_cfun1_pure(cfun1_use));

    // for now, testing
    hash_stdin    = symbol_set("#stdin",    cell_rchannel());
}

static void cfun_exit(void) {

    // loose circular definitions
    oblist_set(hash_f, NIL);
    oblist_set(hash_t, NIL);
    oblist_set(hash_void, NIL);
    oblist_set(hash_undef, NIL);
}
