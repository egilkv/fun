/*  TAB-P
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

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

// make integer, truncating
// false if overflow
int make_integer(number *np) {
    switch (np->divisor) {
    case 0:
	// overflow detection
        // TODO nextafter() should be a constant
        if (np->dividend.fval > nextafter(LLONG_MAX, 0) || np->dividend.fval < nextafter(LLONG_MIN, 0)) {
            return 0;
	}
        np->dividend.ival = np->dividend.fval;
        np->divisor = 1;
        break;

    default: // quotient
        np->dividend.ival /= np->divisor;
        np->divisor = 1;
	break;

    case 1:
        break; // int already
    }
    return 1;
}

// round to nearest integer
int round_integer(number *np) {
    switch (np->divisor) {
    case 0:
        // rounding
        np->dividend.fval += (np->dividend.fval < 0.0) ? -0.5 : 0.5;
        return make_integer(np);

    default: // quotient
	{
            integer_t mod = np->dividend.ival % np->divisor;
            np->dividend.ival /= np->divisor;
	    if (mod >= 0) {
		// 4 % 3 ==> 1
                if (mod*2 >= np->divisor) ++np->dividend.ival; // round up
	    } else {
		// -4 % 3 ==> -1
                if (-mod*2 >= np->divisor) --np->dividend.ival; // round "up"
	    }
            np->divisor = 1;
	}
        return 1;

    case 1:
        return 1;
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
// false if overflow
int normalize_q(number *np) {
    if (np->divisor < 0) { // TODO only possible after a division?
        // cannot overflow
        np->divisor = -np->divisor; // divisor always positive
#ifdef __GNUC__
        if (__builtin_ssubll_overflow(0,
                                      np->dividend.ival,
                                      &(np->dividend.ival))) {
            return 0;
        }
#else
        // no overflow detection
        np->dividend.ival = -np->dividend.ival;
#endif
    }
    if (np->divisor > 1) {
        integer_t c = gcd(np->dividend.ival, np->divisor);
        if (c > 1) {
            // cannot overflow
            np->dividend.ival /= c;
            np->divisor /= c;
        }
    }
    return 1;
}

int make_negative(number *np) {
    if (np->divisor == 0) {
        // cannot overflow, we believe
        np->dividend.fval = -np->dividend.fval;
    } else {
#ifdef __GNUC__
        if (__builtin_ssubll_overflow(0,
                                      np->dividend.ival,
                                      &(np->dividend.ival))) {
            return 0;
        }
#else
        // no overflow detection
        np->dividend.ival = -np->dividend.ival;
#endif
    }
    return 1;
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

