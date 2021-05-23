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
    if (!list_split2(&args, ap)) {
	assert(args == NIL);
	*ap = error_rt0("missing 1st argument");
	return 0;
    }
    if (bp && !list_split2(&args, bp)) {
	assert(args == NIL);
	cell_unref(*ap);
	*ap = error_rt0("missing 2nd argument");
	return 0;
    }
    if (cp && !list_split2(&args, cp)) {
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

// does not consume a
cell *ref_index(cell *a, index_t index) {
    cell *value;
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
	    value = cell_astring(s, 1);
        } else {
            return error_rti("string index out of bounds", index);
	}
	return value;

    case c_LIST:
	value = NIL;
	{
	    index_t i = 0;
	    do {
		if (!cell_is_list(a)) {
		    return error_rti("list index out of bounds", index);
		}
		value = cell_car(a);
		a = cell_cdr(a);
	    } while (i++ < index);
	}
	return cell_ref(value);

    case c_PAIR:
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
        return value;

    default:
    // TODO ref should work for functions ??
    case c_FUNC:
    case c_SPECIAL:
    case c_SYMBOL:
    case c_INTEGER:
    case c_ASSOC:
	 break;
    }
    return error_rt1("cannot referrence", cell_ref(a));
}

