/*  TAB-P
 *
 *  module math
 */

#include "cmod.h"
#include "err.h"
#include "number.h"
#include "m_string.h"

static cell *string_assoc = NIL;

static cell *cstr_length(cell *a) {
    char_t *ptr;
    index_t len;
    cell *result;
    if (!peek_string(a, &ptr, &len, NIL)) return cell_ref(hash_void); // error
    return cell_integer(len);

    cell_unref(a);
    return result;
}

// TODO is this OK
static cell *cstr_ordinal(cell *a) {
    char_t *ptr;
    index_t len;
    cell *result;
    if (!peek_string(a, &ptr, &len, NIL)) return cell_ref(hash_void); // error
    if (len < 1) {
        result = cell_ref(hash_void); // TODO message?
    } else {
        result = cell_integer(ptr[0]);
    }
    cell_unref(a);
    return result;
}

cell *module_string() {
    if (!string_assoc) {
	string_assoc = cell_assoc();

        // TODO these functions are impure
        assoc_set(string_assoc, cell_symbol("length"),  cell_cfun1(cstr_length));
        assoc_set(string_assoc, cell_symbol("ordinal"), cell_cfun1(cstr_ordinal));
    }
    // TODO static string_assoc owns one, hard to avoid
    return cell_ref(string_assoc);
}


