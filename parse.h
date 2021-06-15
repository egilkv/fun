/*  TAB-P
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
    l_PERHAPS,  // labels perhaps allowed (ugly, I know)
    l_LABEL,    // labels allowed here
    l_BASE,     // base expression, no labels allowed
    l_SEMI,     // ;            TODO new, same as ',' in C
    l_RANGE,    // ..           TODO new
    l_DEF,      // =            right-to-left
    l_COND,     // ?            TODO right-to-left, in C ? : are same
    l_OR ,      // ||           TODO
    l_AND,      // &&
    l_BAR,      // |            TODO revise
    l_AMP,      // &            TODO revise
    l_CAT,      // ++           TODO where?
    l_EQEQ,     // == !=
    l_REL,      // < <= > >=
    l_ADD,      // + -
    l_MULT,     // * /
    l_UNARY,    // - ! '        TODO right-to-left
    l_POST,     // () []        as postfix operators
    l_STOP      // .            left-to-right, like C
};

typedef enum prec_ precedence;

void interactive_mode(const char *greeting, const char *prompt);

cell *expression(lxfile *in);

cell *chomp_lx(lxfile *lxf, const char *prompt);

int chomp_file(const char *name, cell **resultp);

