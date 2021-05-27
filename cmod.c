/*  TAB-P
 *
 *  C binding helpers
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cfun.h"
#include "oblist.h"
#include "err.h"

cell *hash_f;
cell *hash_t;
cell *hash_void;
cell *hash_undefined;

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
    return arg3(args, ap, NULL, NULL);
}

// function with 2 arguments
// if false, *ap is error value
int arg2(cell *args, cell **ap, cell **bp) {
    return arg3(args, ap, bp, NULL);
}

// a in always unreffed
// dump is unreffed only if error
int get_number(cell *a, number *np, cell *dump) {
    if (!cell_is_number(a)) {
	cell_unref(dump);
	cell_unref(error_rt1("not a number", a));
	return 0;
    }
    *np = a->_.n;
    cell_unref(a);
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

// as peek_string, but nul-terminated C string is returned
int peek_cstring(cell *a, char **valuep, cell *dump) {
    index_t dummy;
    return peek_string(a, valuep, &dummy, dump);
}

// a in always unreffed
// dump is unreffed only if error
int get_boolean(cell *a, int *boolp, cell *dump) {
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
	    char_t *s = malloc(1 + 1);
	    assert(s);
	    s[0] = a->_.string.ptr[index];
	    s[1] = '\0';
            return cell_astring(s, 1);
        } else {
            return error_rti("string index out of bounds", index);
	}

    case c_LIST:
        if (!list_spin(&a, index)) return a;
        if (!a) {
            return error_rti("list index out of bounds", index);
        }
        return cell_ref(cell_car(a));

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
            char_t *s;
            if (index > a->_.string.len) {
                return error_rti("string range out of bounds", index);
            }
            if (index+len > a->_.string.len) {
                return error_rti("string range out of bounds", index+len-1);
            }
            // TODO special case if whole string
            s = malloc((len+1) * sizeof(char_t));
	    assert(s);
            memcpy(s, &(a->_.string.ptr[index]), (len+1) * sizeof(char_t)); // including '\0'
            return cell_astring(s, len);
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
                value = cell_vector(len);
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



