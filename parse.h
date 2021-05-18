/* TAB-P
 *
 */

#include "cell.h"
#include "lex.h"

// TODO see also:
// https://www.tutorialspoint.com/cprogramming/c_operators_precedence.htm
//
// all associate left-to-right except where noted
// several of these are N-ary
enum prec_ {
    l_BOT,
    l_SEMI,     // ;            TODO new, same as ',' in C
    l_DEF,      // =            right-to-left
    l_COND,     // ?            TODO right-to-left, in C ? : are same
    l_COLON,    // :            TODO
    l_OR ,      // ||           TODO
    l_AND,      // &&
    l_BAR,      // |            TODO revise
    l_AMP,      // &            TODO revise
    l_EQEQ,     // == !=
    l_REL,      // < <= > >=
    l_ADD,      // + -
    l_MULT,     // * /
    l_UNARY,    // - !          TODO right-to-left
    l_POST,     // () []
    l_STOP      // .            left-to-right, like C
};

typedef enum prec_ precedence;

cell *expression(lxfile *in);

