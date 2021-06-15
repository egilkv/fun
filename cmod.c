/*  TAB-P
 *
 *  C binding helpers
 *  and other useful things
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cfun.h"
#include "oblist.h"
#include "number.h"
#include "err.h"

cell *hash_f;
cell *hash_t;
cell *hash_void;
cell *hash_undef;
cell *hash_ellip;

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
    if (!list_pop(&args, ap)) {
	assert(args == NIL);
	*ap = error_rt0("missing 1st argument");
	return 0;
    }
    if (bp && !list_pop(&args, bp)) {
	assert(args == NIL);
	cell_unref(*ap);
	*ap = error_rt0("missing 2nd argument");
	return 0;
    }
    if (cp && !list_pop(&args, cp)) {
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
    return arg3(args, ap, NILP, NILP);
}

// function with 2 arguments
// if false, *ap is error value
int arg2(cell *args, cell **ap, cell **bp) {
    return arg3(args, ap, bp, NILP);
}

// a is never unreffed
// dump is unreffed only if error
int peek_number(cell *a, number *np, cell *dump) {
    if (!cell_is_number(a)) {
	cell_unref(dump);
        cell_unref(error_rt1("not a number", cell_ref(a)));
	return 0;
    }
    *np = a->_.n;
    return 1;
}

// a in always unreffed
// dump is unreffed only if error
int get_number(cell *a, number *np, cell *dump) {
    int ok = peek_number(a, np, dump);
    cell_unref(a);
    return ok;
}

// get a number, convert to integer
// dump is unreffed only if error
int get_any_integer(cell *a, integer_t *valuep, cell *dump) {
    number nn;
    if (!get_number(a, &nn, dump)) {
	return 0;
    }
    if (!make_integer(&nn)) {
        cell_unref(err_overflow(dump));
        return 0;
    }
    *valuep = nn.dividend.ival;
    return 1;
}

// a in always unreffed
// dump is unreffed only if error
int get_integer(cell *a, integer_t *valuep, cell *dump) {
    if (!cell_is_integer(a)) {
	cell_unref(dump);
        cell_unref(error_rt1("not an integer", a));
	return 0;
    }
    *valuep = a->_.n.dividend.ival;
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

// a in not unreffed unless there is an error (we need the string to live)
// dump is also unreffed only if error
int peek_string(cell *a, char_t **valuep, index_t *lengthp, cell *dump) {
    if (a) switch (a->type) {
    case c_STRING:
	*valuep = a->_.string.ptr;
        *lengthp = a->_.string.len;
	return 1;
    default:
	break;
    }
    cell_unref(dump);
    (error_rt1("not a string", a));
    return 0;
}

// a and dump is unreffed only if error
int peek_symbol(cell *a, char_t **valuep, cell *dump) {
    if (a) switch (a->type) {
    case c_SYMBOL:
	*valuep = a->_.symbol.nam;
	return 1;
    default:
	break;
    }
    cell_unref(dump);
    cell_unref(error_rt1("not a symbol", a));
    return 0;
}

// a in always unreffed
// dump is unreffed only if error
int get_symbol(cell *a, char_t **valuep, cell *dump) {
    if (!peek_symbol(a, valuep, dump)) return 0;
    cell_unref(a);
    return 1;
}

// as peek_string, but nul-terminated C string is returned
int peek_cstring(cell *a, char **valuep, cell *dump) {
    index_t dummy;
    return peek_string(a, valuep, &dummy, dump);
}

// a in never unreffed, and no errors
int peek_boolean(cell *a, int *boolp) {
    if (a == hash_t) {
        *boolp = 1;
        return 1;
    } else if (a == hash_f) {
        *boolp = 0;
        return 1;
    }
    return 0;
}

// a in always unreffed
// dump is unreffed only if error
int get_boolean(cell *a, int *boolp, cell *dump) {
    if (!peek_boolean(a, boolp)) {
        cell_unref(dump);
        cell_unref(error_rt1("not a boolean", a));
        return 0;
    }
    cell_unref(a);
    return 1;
}

// arg is unreffed, unless there is an error
int peek_special(const char *magic, cell *arg, void **valuep, cell *dump) {
    if (!cell_is_special(arg, magic)) {
        const char *m = magic ? magic : "special";
        char *errmsg = malloc(strlen(m) + 6);
        assert(errmsg);
        strcpy(errmsg, "not a ");
        strcpy(errmsg+6, m);
        cell_unref(error_rt1(errmsg, cell_ref(arg)));
        free(errmsg);
        cell_unref(arg);
        cell_unref(dump);
        return 0;
    }
    *valuep = arg->_.special.ptr;
    return 1;
}

// a in always unreffed
// dump is unreffed only if error
int get_special(const char *magic, cell *arg, void **valuep, cell *dump) {
    if (!peek_special(magic, arg, valuep, dump)) {
        return 0;
    }
    cell_unref(arg);
    return 1;
}

// does not consume
integer_t ref_length(cell *a) {
    switch (a ? a->type : c_LIST) {

    case c_VECTOR:
        return a->_.vector.len;

    case c_STRING:
        return a->_.string.len;

    case c_LIST:
	{
            index_t length = 0;
            while (a) {
                ++length;
                assert(cell_is_list(a));
		a = cell_cdr(a);
            }
            return length;
	}

    default:
        return -1; // cannot find length
    }
}

// does not consume
// on error, return zero and set *ap to reffed void
static int list_spin(cell **ap, index_t times) {
    cell *a = *ap;
    index_t i;
    for (i = 0; i < times; ++i) {
        if (!a) {
            *ap = error_rti("list index out of bounds", times);
            return 0;
        }
        assert(cell_is_list(a));
        a = cell_cdr(a);
    }
    *ap = a;
    return 1;
}

// high level reference function, does it all
// consumes both arguments, produces error messages
cell *cfun2_ref(cell *a, cell *b) {
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
            if (!get_index(b1, &index1, b2)) {
                cell_unref(a);
                return cell_error();
            }
        }
        if (b2) {
            index_t index2 = 0;
            if (!get_index(b2, &index2, a)) return cell_error();
            if (index2 < index1) {
                return error_rti("index range cannot be reverse", index2);
            }
            value = ref_range2(a, index1, 1+index2 - index1);
        } else {
            value = ref_range1(a, index1);
        }
    } else {
	index_t index;
        if (!get_index(b, &index, a)) return cell_error();
	value = ref_index(a, index);
    }
    cell_unref(a);
    return value;
}

// does not consume
cell *ref_index(cell *a, index_t index) {
    cell *value = NIL;
    if (a) switch (a->type) {

    case c_VECTOR:
	if (!vector_get(a, index, &value)) {
            value = error_rti("vector index out of bounds", index);
	}
	return value;

    case c_STRING:
	if (index < a->_.string.len) {
            return cell_nastring(&(a->_.string.ptr[index]), 1);
        } else {
            return error_rti("string index out of bounds", index);
	}

    case c_LIST:
        if (!list_spin(&a, index)) return a;
        if (!a) {
            return error_rti("list index out of bounds", index);
        }
        return cell_ref(cell_car(a));

    case c_LABEL:
        switch (index) {
        case 0:
            return cell_ref(cell_car(a));
        case 1:
            return cell_ref(cell_cdr(a));
        default:
            break;
        }
        return error_rti("label index out of bounds", index);

    default:
        return error_rt1("cannot referrence", cell_ref(a));
    } else {
        return error_rt1("cannot referrence", NIL);
    }
}

// does not consume
cell *ref_range1(cell *a, index_t index) {
    switch (a ? a->type : c_LIST) {

    case c_STRING:
        return ref_range2(a, index, a->_.string.len - index);

    case c_VECTOR:
        return ref_range2(a, index, a->_.vector.len - index);

    case c_LIST:
        if (!list_spin(&a, index)) return a;
        // NIL list is tolerated here
        return cell_ref(a); 

    default:
        return ref_range2(a, index, 0);
    }
}

// does not consume
cell *ref_range2(cell *a, index_t index, integer_t len) {
    switch (a ? a->type : c_LIST) {

    case c_STRING:
        {
            if (index > a->_.string.len) {
                return error_rti("string range out of bounds", index);
            }
            if (index+len > a->_.string.len) {
                return error_rti("string range out of bounds", index+len-1);
            }
            // TODO special case if whole string
            return cell_nastring(&(a->_.string.ptr[index]), len);
	}

    case c_VECTOR:
        {
            cell *value = NIL;
            integer_t i;
            if (index > a->_.vector.len) {
                return error_rti("vector range out of bounds", index);
            }
            if (index+len > a->_.vector.len) {
                return error_rti("vector range out of bounds", index+len-1);
            }
            if (len > 0) {
                value = cell_vector_nil(len);
                for (i = 0; i < len; ++i) {
                    value->_.vector.table[i] = cell_ref(a->_.vector.table[index+i]);
                }
            } else {
                value = NIL; // empty vector is always NIL
            }
            return value;
	}

    case c_LIST:
        {
            integer_t i;
            cell *value = NIL;
            cell **pp = &value;
            if (!list_spin(&a, index)) return a;
            // make copy of len items of remaining list
            // TODO can optimize if all of remaining list
            for (i=0; i < len; ++i) {
                if (!a) {
                    cell_unref(value); // anything so far
                    return error_rti("list range out of bounds", index+len-1);
                }
                assert(cell_is_list(a));
                *pp = cell_list(cell_ref(cell_car(a)), NIL);
                pp = &((*pp)->_.cons.cdr);
                a = cell_cdr(a);
            }
            return value;
	}

    default:
        return error_rt1("cannot referrence", cell_ref(a));
    } 
}

// TODO inline
cell *cell_void() {
    return cell_ref(hash_void);
}

// TODO do something smarter
cell *cell_error() {
    return cell_ref(hash_void);
}

// check if item exists on list
// refs nothing
int exists_on_list(cell *list, cell *item) {
    cell *param;
    while (list) {
        assert(cell_is_list(list));
        param = cell_car(list);
        if (cell_is_label(param)) param = param->_.cons.car;
        if (param == item) return 1; // found it
        list = cell_cdr(list);
    }
    return 0;
}

cell *defq(cell *nam, cell *val, cell **envp) {
    if (!cell_is_symbol(nam)) {
        cell_unref(val);
        return error_rt1("not a symbol", nam);
    }
    if (*envp) {
        if (!assoc_set_local(env_assoc(*envp), nam, cell_ref(val))) {
            cell_unref(val);
            cell_unref(error_rt1("cannot redefine immutable", nam));
        }
    } else {
	// TODO mutable
        oblist_set(nam, cell_ref(val));
        cell_unref(nam);
    }
    return val;
}
