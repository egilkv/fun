/*  TAB-P
 *
 *  module string
 *
 *  suggested functions:
 *  str.pos(haystack, needle) -> pos or #void
 *  str.explode("a&b&c", "&") -> ["a", "b", "c"]
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cmod.h"
#include "err.h"
#include "number.h"
#include "m_string.h"

static cell *cstr_len(cell *a) {
    char_t *ptr;
    index_t len;
    cell *result;
    if (!peek_string(a, &ptr, &len, NIL)) return cell_error();
    return cell_integer(len);

    cell_unref(a);
    return result;
}

static cell *cstr_lower(cell *a) {
    char_t *ptr;
    char_t *ptr2;
    index_t n;
    index_t len;
    cell *result;
    if (!peek_string(a, &ptr, &len, NIL)) return cell_error();
    ptr2 = malloc(len+1);
    assert(ptr2);
    for (n = 0; n < len; ++n) {
        ptr2[n] = tolower(ptr[n]);
    }
    ptr2[len] = '\0';
    // TODO can optimize if no case conversion happened
    result = cell_astring(ptr2, len);
    cell_unref(a);
    return result;
}

static cell *cstr_upper(cell *a) {
    char_t *ptr;
    char_t *ptr2;
    index_t n;
    index_t len;
    cell *result;
    if (!peek_string(a, &ptr, &len, NIL)) return cell_error();
    ptr2 = malloc(len+1);
    assert(ptr2);
    for (n = 0; n < len; ++n) {
        ptr2[n] = toupper(ptr[n]);
    }
    ptr2[len] = '\0';
    // TODO can optimize if no case conversion happened
    result = cell_astring(ptr2, len);
    cell_unref(a);
    return result;
}

// TODO use Boyer-Moore
static cell *cstr_pos(cell *a, cell *b) {
    char_t *hayptr, *ndlptr;
    index_t haylen, ndllen;
    char_t *found;
    integer_t result = -1;
    if (!peek_string(a, &hayptr, &haylen, b)
     || !peek_string(b, &ndlptr, &ndllen, a)) return cell_error();
    if (ndllen <= 0) {
        result = 0;
    } else if (ndllen == 1) {
        if (haylen >= 1
         && (found = memchr(hayptr, ndlptr[0], haylen)) != NULL) {
            result = found-hayptr;
            assert(result < haylen);
        }
    } else {
        char_t *hayptr0 = hayptr;
        while (haylen >= ndllen
            && (found = memchr(hayptr, ndlptr[0], haylen)) != NULL) {
            haylen -= found-hayptr;
            hayptr = found;
            if (haylen < ndllen) break; // no way
            if (memcmp(hayptr+1, ndlptr+1, ndllen-1) == 0) { // found it?
                result = hayptr-hayptr0;
                break;
            }
            ++hayptr;
            --haylen;
        }
    }
    cell_unref(a);
    cell_unref(b);
    return result < 0 ? cell_ref(hash_void) : cell_integer(result);
}

static cell *cstr_char(cell *a) {
    integer_t chr;
    char str;
    if (!get_integer(a, &chr, NIL)) return cell_error();
    if (chr < 0 || chr > 255) return error_rti("chr out of range", chr);
    str = chr;
    return cell_nastring(&str, 1);
}

// TODO is this OK?
static cell *cstr_ordinal(cell *a) {
    char_t *ptr;
    index_t len;
    cell *result;
    if (!peek_string(a, &ptr, &len, NIL)) return cell_error();
    if (len < 1) {
        result = cell_void();
    } else {
        result = cell_integer(ptr[0]);
    }
    cell_unref(a);
    return result;
}

cell *module_string() {
    // TODO consider cache
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("char"),    cell_cfun1_pure(cstr_char));
    assoc_set(a, cell_symbol("len"),     cell_cfun1_pure(cstr_len)); // TODO already covered?
    assoc_set(a, cell_symbol("lower"),   cell_cfun1_pure(cstr_lower));
    assoc_set(a, cell_symbol("ordinal"), cell_cfun1_pure(cstr_ordinal));
    assoc_set(a, cell_symbol("pos"),     cell_cfun2_pure(cstr_pos));
    assoc_set(a, cell_symbol("upper"),   cell_cfun1_pure(cstr_upper));

    return a;
}

