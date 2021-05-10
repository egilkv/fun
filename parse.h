/* TAB-P
 *
 */

#include "cell.h"

// TODO see also:
// https://www.tutorialspoint.com/cprogramming/c_operators_precedence.htm
//
// all associate left to right except where noted
enum prec_ {
    l_BOT,
    l_COMMA,    // ,            TODO
    l_DEF,      // =            TODO right to left
    l_COND,     // ?            TODO right to left, in C ? : are same
    l_COLO,     // :            TODO
    l_OR ,      // ||           TODO
    l_AND,      // &&
    l_AMP,      // & TODO revise
    l_EQ,       // == !=
    l_REL,      // < <= > >=
    l_ADD,      // + -
    l_MULT,     // * /
    l_UNARY,    // + - !        TODO right to left etc
    l_POST      // . TODO
};

typedef enum prec_ precedence;

cell *expression();

