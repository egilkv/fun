/*  TAB-P
 *
 *  basic types
 */

#ifndef TYPE_H

// TODO review size_t and so on, ssize_t is signed size

typedef unsigned long long int index_t; // TODO 64 bit
typedef long long int integer_t; // TODO 64 bit
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
