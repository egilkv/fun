/*  TAB-P
 *
 */

#include "cell.h"
#include "oblist.h"
#include "number.h"
#include "set.h"
#include "cmod.h"
#include "err.h"

// in case we need to define symbol
static INLINE cell *sp_key(set *sp) {
    if (sp->key == NIL) sp->key = symbol_peek(sp->str);
    return sp->key;
}

// TODO allow integers?
// key is always unreffed
int get_fromset(set *sp, cell *key, integer_t *ivalp) {
    while (sp->str) {
        if (key == sp_key(sp)) {
            *ivalp = sp->ival;
            cell_unref(key);
            return 1;
        }
        ++sp;
    }
    cell_unref(error_rt1("not in set", key)); // TODO improve
    return 0;
}

static int peek_fromset(set *sp, integer_t ival, cell **cp) {
    while (sp->str) {
        if (ival == sp->ival) {
            *cp = sp_key(sp);
            return 1;
        }
        ++sp;
    }
    return 0;
}

// get reffed symbol from set
cell *cell_fromset(set *sp, integer_t ival) {
    cell *result = NIL;
    if (peek_fromset(sp, ival, &result)) {
        return cell_ref(result);
    }
    // TODO create error instead?
    return cell_integer(ival);
}

// get reffed list from a set that is a mask
// TODO there really should be a better way
cell *cell_maskset(set *sp, integer_t ival) {
    cell *result = NIL;
    integer_t mask = 1;
    while (ival != 0 && mask != 0) {
        if ((ival & mask) != 0) {
            cell *c = NIL;
            if (peek_fromset(sp, mask, &c)) {
                result = cell_list(cell_ref(c), result);
            } else {
                // TODO do something else?
                result = cell_list(cell_integer(mask), result);
            }
            ival &= ~mask;
        }
        mask <<= 1;
    }
    return result;
}


// TODO allow integers?
// key is always unreffed
int get_maskset(set *sp, cell *keys, integer_t *ivalp) {
    if (keys == NIL || cell_is_list(keys)) {
        cell *k;
        integer_t result = 0;
        while (list_pop(&keys, &k)) {
            integer_t val = 0;
            if (get_fromset(sp, k, &val)) {
                // TODO more checking...
                result |= val;
            }
        }
        *ivalp = result;
        return 1;
    } else {
        return get_fromset(sp, keys, ivalp);
    }
}

