/*  TAB-P
 *
 *  bit manipulation
 */

#include <assert.h>

#include "m_bit.h"
#include "number.h"
#include "cmod.h"

// TODO should it also work for booleans? possibly
static cell *cbit_and(cell *args) {
    integer_t result;
    integer_t operand;
    cell *a;
    if (!list_pop(&args, &a)
     || !get_integer(a, &result, args)) return cell_void(); // error
    while (list_pop(&args, &a)) {
        if (!get_integer(a, &operand, args)) return cell_void();
        result &= operand;
    }
    assert(args == NIL);
    return cell_integer(result);
}

static cell *cbit_or(cell *args) {
    integer_t result;
    integer_t operand;
    cell *a;
    if (!list_pop(&args, &a)
     || !get_integer(a, &result, args)) return cell_void(); // error
    while (list_pop(&args, &a)) {
        if (!get_integer(a, &operand, args)) return cell_void();
        result |= operand;
    }
    assert(args == NIL);
    return cell_integer(result);
}

static cell *cbit_xor(cell *args) {
    integer_t result;
    integer_t operand;
    cell *a;
    if (!list_pop(&args, &a)
     || !get_integer(a, &result, args)) return cell_void(); // error
    while (list_pop(&args, &a)) {
        if (!get_integer(a, &operand, args)) return cell_void();
        result ^= operand;
    }
    assert(args == NIL);
    return cell_integer(result);
}

static cell *cbit_not(cell *a) {
    integer_t result;
    if (!get_integer(a, &result, NIL)) return cell_void(); // error
    result = ~result;
    return cell_integer(result);
}

// positive is left shift
static cell *cbit_shift(cell *a, cell *b) {
    integer_t result;
    integer_t shift;
    if (!get_integer(a, &result, b)
     || !get_integer(b, &shift, NIL)) return cell_void(); // error
    // TODO overflow
    if (shift > 0) result <<= shift;
    else if (shift < 0) result >>= -shift;
    return cell_integer(result);
}

cell *module_bit() {
    // TODO consider cache
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("and"),    cell_cfunN(cbit_and));
    assoc_set(a, cell_symbol("not"),    cell_cfun1(cbit_not));
    assoc_set(a, cell_symbol("or"),     cell_cfunN(cbit_or));
    assoc_set(a, cell_symbol("xor"),    cell_cfunN(cbit_xor));
    assoc_set(a, cell_symbol("shift"),  cell_cfun2(cbit_shift));

    return a;
}

