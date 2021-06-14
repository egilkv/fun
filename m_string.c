/*  TAB-P
 *
 *  module math
 */

#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include "cmod.h"
#include "err.h"
#include "number.h"
#include "m_string.h"

static cell *cstr_length(cell *a) {
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

// TODO is this OK?
static cell *cstr_ordinal(cell *a) {
    char_t *ptr;
    index_t len;
    cell *result;
    if (!peek_string(a, &ptr, &len, NIL)) return cell_error();
    if (len < 1) {
        result = cell_void(); // TODO message?
    } else {
        result = cell_integer(ptr[0]);
    }
    cell_unref(a);
    return result;
}

cell *module_string() {
    // TODO consider cache
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("length"),  cell_cfun1(cstr_length));
    assoc_set(a, cell_symbol("lower"),   cell_cfun1(cstr_lower));
    assoc_set(a, cell_symbol("ordinal"), cell_cfun1(cstr_ordinal));
    assoc_set(a, cell_symbol("upper"),   cell_cfun1(cstr_upper));

    return a;
}


