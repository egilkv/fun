/*  TAB-P
 *
 *  module math
 */

#include <math.h>

#include "cmod.h"
#include "number.h"
#include "err.h"
#include "m_math.h"

static cell *math_assoc = NIL;

static cell *cmath_div(cell *a, cell *b) {
    integer_t ia;
    integer_t ib;
    if (!get_integer(a, &ia, b)
     || !get_integer(b, &ib, NIL)) {
        return cell_void(); // error
    }
    if (ib == 0) {
        return error_rt0("attempted division by zero");
    }
    ia /= ib;
    return cell_integer(ia);
}

static cell *cmath_mod(cell *a, cell *b) {
    integer_t ia;
    integer_t ib;
    if (!get_integer(a, &ia, b)
     || !get_integer(b, &ib, NIL)) {
        return cell_void(); // error
    }
    if (ib == 0) {
        return error_rt0("attempted modulus zero");
    }
    ia %= ib;
    return cell_integer(ia);
}

static cell *cmath_abs(cell *a) {
    number na;
    if (!peek_number(a, &na, a)) return cell_void(); // error

    // TODO can optimize to return argument if >= 0
    if (na.divisor == 0) {
        if (na.dividend.fval >= 0.0) return a;
    } else {
        if (na.dividend.ival >= 0) return a;
    }
    cell_unref(a);
    if (!make_negative(&na)) {
        return err_overflow(NIL);
    }
    return cell_number(&na);
}

// convert to integer, rounding to closest
static cell *cmath_int(cell *a) {
    number na;
    if (!peek_number(a, &na, a)) return cell_void(); // error

    switch (na.divisor) {
    default:
        na.dividend.fval = (1.0 * na.dividend.ival) / na.divisor;
        // fall through...
    case 0:
        if (na.dividend.fval > 0.0) {
            na.dividend.fval += 0.5;
        } else if (na.dividend.fval < 0.0) {
            na.dividend.fval -= 0.5;
        }
        // TODO overflow detection how?
        na.dividend.ival = na.dividend.fval;
        na.divisor = 1;
        break;
    case 1:
        return a; // int already
    }
    cell_unref(a);
    return cell_number(&na);
}

#if HAVE_MATH

static cell *cmath_sqrt(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = sqrt(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("no real square root");
    return cell_number(&na);
}

static cell *cmath_pow(cell *a, cell *b) {
    number na;
    number nb;
    if (!get_float(a, &na, b)
     || !get_float(b, &nb, NIL)) {
        return cell_void(); // error
    }
    // TODO handle special cases of integer power?
    na.dividend.fval = pow(na.dividend.fval, nb.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("pow out of range");
    return cell_number(&na);
}

static cell *cmath_log(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = log(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("log undefined for negative argument");
    return cell_number(&na);
}

static cell *cmath_log10(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = log10(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("log undefined for negative argument");
    return cell_number(&na);
}

static cell *cmath_cos(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = cos(na.dividend.fval);
    return cell_number(&na);
}

static cell *cmath_sin(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = sin(na.dividend.fval);
    return cell_number(&na);
}

static cell *cmath_tan(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = tan(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("tan out of range"); // TODO investige'
    return cell_number(&na);
}

static cell *cmath_acos(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    make_float(&na);
    na.dividend.fval = acos(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) {
        return error_rt0("acos out of range");
    }
    return cell_number(&na);
}

static cell *cmath_asin(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = asin(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("asin out of range");
    return cell_number(&na);
}

static cell *cmath_atan(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = atan(na.dividend.fval);
    return cell_number(&na);
}

static cell *cmath_atan2(cell *y, cell *x) {
    number ny;
    number nx;
    if (!get_float(y, &ny, x)
     || !get_float(x, &nx, NIL)) return cell_void(); // error
    ny.dividend.fval = atan2(ny.dividend.fval, nx.dividend.fval);
    return cell_number(&ny);
}

static cell *cmath_ceil(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = ceil(na.dividend.fval);
    return cell_number(&na);
}

static cell *cmath_floor(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_void(); // error
    na.dividend.fval = floor(na.dividend.fval);
    return cell_number(&na);
}

#endif // HAVE_MATH

cell *module_math() {
    if (!math_assoc) {
        math_assoc = cell_assoc();

        // TODO these functions are impure
        assoc_set(math_assoc, cell_symbol("abs"),   cell_cfun1(cmath_abs));
        assoc_set(math_assoc, cell_symbol("div"),   cell_cfun2(cmath_div));
        assoc_set(math_assoc, cell_symbol("e"),     cell_real(M_E));
        assoc_set(math_assoc, cell_symbol("int"),   cell_cfun1(cmath_int));
        assoc_set(math_assoc, cell_symbol("mod"),   cell_cfun2(cmath_mod));
        assoc_set(math_assoc, cell_symbol("pi"),    cell_real(M_PI));
#if HAVE_MATH
        assoc_set(math_assoc, cell_symbol("acos"),  cell_cfun1(cmath_acos));
        assoc_set(math_assoc, cell_symbol("asin"),  cell_cfun1(cmath_asin));
        assoc_set(math_assoc, cell_symbol("atan"),  cell_cfun1(cmath_atan));
        assoc_set(math_assoc, cell_symbol("atan2"), cell_cfun2(cmath_atan2));
        assoc_set(math_assoc, cell_symbol("ceil"),  cell_cfun1(cmath_ceil));
        assoc_set(math_assoc, cell_symbol("cos"),   cell_cfun1(cmath_cos));
        assoc_set(math_assoc, cell_symbol("floor"), cell_cfun1(cmath_floor));
        assoc_set(math_assoc, cell_symbol("log"),   cell_cfun1(cmath_log));
        assoc_set(math_assoc, cell_symbol("log10"), cell_cfun1(cmath_log10));
        assoc_set(math_assoc, cell_symbol("pow"),   cell_cfun2(cmath_pow));
        assoc_set(math_assoc, cell_symbol("sin"),   cell_cfun1(cmath_sin));
        assoc_set(math_assoc, cell_symbol("sqrt"),  cell_cfun1(cmath_sqrt));
        assoc_set(math_assoc, cell_symbol("tan"),   cell_cfun1(cmath_tan));
#endif // HAVE_MATH
    }
    // TODO static math_assoc owns one, hard to avoid
    return cell_ref(math_assoc);
}

