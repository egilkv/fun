/*  TAB-P
 *
 *  basic types and compiler specific things
 *
 *  this is for 64 bit modern GCC
 *  see also: #ifdef __GNUC__
 */

#ifndef TYPE_H

#define INLINE inline

typedef volatile unsigned refcount_t; // 32 bit TODO will limit # of cells to 50G of memory
typedef volatile int lock_t; // for locking

// TODO review size_t and so on, ssize_t is signed size

typedef unsigned long long int index_t; // TODO 64 bit
typedef long long int integer_t; // TODO 64 bit

// TODO 64 bit limits are LLONG_MAX and LLONG_MIN
typedef double real_t; // 64 bit
typedef char char_t; // TODO unsigned

struct number_s {
    union {
        integer_t ival;
        real_t    fval;
    } dividend;
    integer_t divisor; // 0: float, 1: integer, 2: quotient
};

typedef struct number_s number;

#define TYPE_H
#endif
