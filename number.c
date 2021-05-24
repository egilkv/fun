/*  TAB-P
 *
 */

#include <stdio.h>
#include <string.h>

#include "type.h"
#include "cmod.h"
#include "number.h"
#include "err.h"

cell *cell_integer(integer_t integer) {
    number n;
    n.dividend.ival = integer;
    n.divisor = 1;
    return cell_number(&n);
}

cell *cell_real(real_t real) {
    number n;
    n.dividend.fval = real;
    n.divisor = 0;
    return cell_number(&n);
}

// make number float
// TODO inline
void make_float(number *np) {
    switch (np->divisor) {
    case 0:
        break; // float already
    case 1: // integer
        np->dividend.fval = 1.0 * np->dividend.ival;
        np->divisor = 0;
        break;
    default: // quotient
        np->dividend.fval = np->dividend.ival / (1.0 * np->divisor);
        np->divisor = 0;
        break;
    }
}

// make both numbers float if one is, and return true if float
int sync_float(number *n1, number *n2) {
    if (n1->divisor == 0) { // n1 is float?
        make_float(n2);
        return 1;
    } else if (n2->divisor == 0) { // n2 is float?
        make_float(n1);
        return 1;
    } else {
        return 0;
    }
}

// a in always unreffed
// dump is unreffed only if error
int get_float(cell *a, number *np, cell *dump) {
    if (!get_number(a, np, dump)) return 0;
    make_float(np);
    return 1;
}

// used Euclidean algorithm
static integer_t gcd(integer_t a, integer_t b) {
    return (b == 0) ? a : gcd(b, a % b);
}

// normalize quotient, i.e. remove gcd component
void normalize_q(number *np) {
    if (np->divisor < 0) { // TODO only possible after a division????
        np->divisor = -np->divisor; // divisor always positive
        np->dividend.ival = -np->dividend.ival;
    }
    if (np->divisor > 1) {
        integer_t c = gcd(np->dividend.ival, np->divisor);
        if (c > 1) {
            np->dividend.ival /= c;
            np->divisor /= c;
        }
    }
}

// buf is of size FORMAT_REAL_LEN
void format_real(real_t r, char *buf) {
    buf[0] = '\0';
    buf[FORMAT_REAL_LEN-1] = '\0';
    snprintf(buf, FORMAT_REAL_LEN-1, "%.15g", r);
    if (!strchr(buf, '.')) {
        // add decimal point
        int n = strlen(buf);
        if (n <= FORMAT_REAL_LEN-3) {
            buf[n] = '.';
            buf[n+1] = '0';
            buf[n+2] = '\0';
        }
    }
}

// report numerical overflow
cell *err_overflow(cell *dump) {
    cell *e = error_rt0("numerical overflow");
    cell_unref(dump);
    return e;
}
