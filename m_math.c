/*  TAB-P
 *
 *  module math
 */
#ifdef HAVE_MATH

#include <math.h>

#include "cmod.h"
#include "number.h"
#include "err.h"
#include "m_math.h"

static cell *cmath_div(cell *a, cell *b) {
    integer_t ia;
    integer_t ib;
    if (!get_integer(a, &ia, b)
     || !get_integer(b, &ib, NIL)) {
        return cell_error();
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
        return cell_error();
    }
    if (ib == 0) {
        return error_rt0("attempted modulus zero");
    }
    ia %= ib;
    return cell_integer(ia);
}

static cell *cmath_abs(cell *a) {
    number na;
    if (!peek_number(a, &na, a)) return cell_error();

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
    if (cell_is_integer(a)) {
        return a;
    }
    if (!get_number(a, &na, NIL)) return cell_error();
    if (!round_integer(&na)) return err_overflow(NIL);
    return cell_number(&na);
}

#if HAVE_MATH

static cell *cmath_sqrt(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = sqrt(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("no real square root");
    return cell_number(&na);
}

static cell *cmath_pow(cell *a, cell *b) {
    number na;
    number nb;
    if (!get_float(a, &na, b)
     || !get_float(b, &nb, NIL)) {
        return cell_error();
    }
    // TODO handle special cases of integer power?
    na.dividend.fval = pow(na.dividend.fval, nb.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("pow out of range");
    return cell_number(&na);
}

static cell *cmath_log(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = log(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("log undefined for negative argument");
    return cell_number(&na);
}

static cell *cmath_log10(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = log10(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("log undefined for negative argument");
    return cell_number(&na);
}

static cell *cmath_cos(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = cos(na.dividend.fval);
    return cell_number(&na);
}

static cell *cmath_sin(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = sin(na.dividend.fval);
    return cell_number(&na);
}

static cell *cmath_tan(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = tan(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("tan out of range"); // TODO investige'
    return cell_number(&na);
}

static cell *cmath_acos(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    make_float(&na);
    na.dividend.fval = acos(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) {
        return error_rt0("acos out of range");
    }
    return cell_number(&na);
}

static cell *cmath_asin(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = asin(na.dividend.fval);
    if (!isfinite(na.dividend.fval)) return error_rt0("asin out of range");
    return cell_number(&na);
}

static cell *cmath_atan(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = atan(na.dividend.fval);
    return cell_number(&na);
}

static cell *cmath_atan2(cell *y, cell *x) {
    number ny;
    number nx;
    if (!get_float(y, &ny, x)
     || !get_float(x, &nx, NIL)) return cell_error();
    ny.dividend.fval = atan2(ny.dividend.fval, nx.dividend.fval);
    return cell_number(&ny);
}

static cell *cmath_ceil(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = ceil(na.dividend.fval);
    return cell_number(&na);
}

static cell *cmath_floor(cell *a) {
    number na;
    if (!get_float(a, &na, NIL)) return cell_error();
    na.dividend.fval = floor(na.dividend.fval);
    return cell_number(&na);
}

#endif // HAVE_MATH

cell *module_math() {
    // TODO consider cache
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("abs"),    cell_cfun1_pure(cmath_abs));
    assoc_set(a, cell_symbol("div"),    cell_cfun2_pure(cmath_div));
    assoc_set(a, cell_symbol("e"),      cell_real(M_E));
    assoc_set(a, cell_symbol("int"),    cell_cfun1_pure(cmath_int));
    assoc_set(a, cell_symbol("mod"),    cell_cfun2_pure(cmath_mod));
    assoc_set(a, cell_symbol("pi"),     cell_real(M_PI));
#if HAVE_MATH
    assoc_set(a, cell_symbol("acos"),   cell_cfun1_pure(cmath_acos));
    assoc_set(a, cell_symbol("asin"),   cell_cfun1_pure(cmath_asin));
    assoc_set(a, cell_symbol("atan"),   cell_cfun1_pure(cmath_atan));
    assoc_set(a, cell_symbol("atan2"),  cell_cfun2_pure(cmath_atan2));
    assoc_set(a, cell_symbol("ceil"),   cell_cfun1_pure(cmath_ceil));
    assoc_set(a, cell_symbol("cos"),    cell_cfun1_pure(cmath_cos));
    assoc_set(a, cell_symbol("floor"),  cell_cfun1_pure(cmath_floor));
    assoc_set(a, cell_symbol("log"),    cell_cfun1_pure(cmath_log));
    assoc_set(a, cell_symbol("log10"),  cell_cfun1_pure(cmath_log10));
    assoc_set(a, cell_symbol("pow"),    cell_cfun2_pure(cmath_pow));
    assoc_set(a, cell_symbol("sin"),    cell_cfun1_pure(cmath_sin));
    assoc_set(a, cell_symbol("sqrt"),   cell_cfun1_pure(cmath_sqrt));
    assoc_set(a, cell_symbol("tan"),    cell_cfun1_pure(cmath_tan));
#endif // HAVE_MATH

    return a;
}

#endif // HAVE_MATH
