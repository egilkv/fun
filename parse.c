/*  TAB-P
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "parse.h"
#include "cfun.h"
#include "number.h"
#include "err.h"
#include "m_io.h" // cell_write

#include "oblist.h"

static cell *expr(precedence lv, lxfile *in);
static cell *getlist(item *op, token sep_token, token end_token, 
                     precedence blv, lxfile *in);
static cell *binary(cell *left, precedence lv, lxfile *in);

static cell *badeof() {
    error_par(" at eof", "unexpected end of file");
    return 0;
}

// return last item, or void if none
cell *chomp_lx(lxfile *lxf) {
    cell *result = cell_void();
    cell *ct;

    for (;;) {
        lxf->show_prompt = 1;
        ct = expression(lxf);
	if (!ct) break; // eof
        if (lxf->f == stdin) {
            if (lxf->show_parse) {
                cell_write(stdout, ct);
                printf(" ==> ");
	    }
	}
        ct = eval(ct, NULL);
        if (lxf->f == stdin) {
            if (ct != hash_void) { // write result if not void
                cell_write(stdout, ct);
                if (!(lxf->is_terminal)) {
                    printf("\n");
                }
            }
	}
        cell_unref(result);
       result = ct;
    }
    if (lxf->is_terminal) {
        printf("\n");
    }
    return result;
}

int chomp_file(const char *name, cell **resultp) {
    cell *result;
    FILE *f = fopen(name, "r");
    lxfile cfile;
    if (!f) {
        return 0;
    }
    lxfile_init(&cfile, f);
    result = chomp_lx(&cfile);
    if (resultp) *resultp = result;
    else cell_unref(result);
    fclose(cfile.f);
    return 1;
}

// get expression at the outer level, where semicolon is separator
cell *expression(lxfile *in) {
    return expr(l_SEMI, in);
}

static cell *expr(precedence lv, lxfile *in) {
    cell *pt = 0;
    item *it;
    it = lexical(in);
    if (!it) return 0; // end of file

    switch (it->type) {

    case it_NUMBER:
        if (it->divisor != 0 && it->ivalue == 0 && it->fvalue > 0.0) {
            // special case: integer has overflowed
            pt = error_rt0("integer number is too large");
        } else if (!isfinite(it->fvalue)) {
            // floating point overflow
            pt = error_rt0("real number is too large");
        } else {
            // regular integer or float
	    number nval;
	    if ((nval.divisor = it->divisor) != 0) {
		nval.dividend.ival = it->ivalue;
            } else {
		nval.dividend.fval = it->fvalue;
            }
	    pt = cell_number(&nval);
        }
        dropitem(it);
        return binary(pt, lv, in);

    case it_STRING:
        pt = cell_astring(it->svalue, it->slen);
        it->svalue = NULL;
        dropitem(it);
        return binary(pt, lv, in);

    case it_SYMBOL:
        pt = cell_asymbol(it->svalue);
        it->svalue = NULL;
        dropitem(it);
        return binary(pt, lv, in);

    case it_NOT: // unary only
        dropitem(it);
        pt = expr(l_UNARY, in);
        if (!pt) return badeof();
        pt = cell_func(cell_ref(hash_not), cell_list(pt, NIL));
        return binary(pt, lv, in);

    case it_TILDE: // unary only
        dropitem(it);
        pt = expr(l_UNARY, in);
        if (!pt) return badeof();
        pt = cell_func(cell_symbol("#bitnot"), cell_list(pt, NIL));
        return binary(pt, lv, in);

    case it_MINUS: // unary and binary
        dropitem(it);
        pt = expr(l_UNARY, in);
        if (!pt) return badeof();
        if (cell_is_number(pt)) {
            // handle unary minus of number here
            if (!make_negative(&(pt->_.n))) {
                return err_overflow(pt);
            }
        } else {
            pt = cell_func(cell_ref(hash_minus), cell_list(pt, NIL));
        }
        return binary(pt, lv, in);

    case it_QUOTE:
        dropitem(it);
        pt = expr(l_UNARY, in);
        if (!pt) return badeof();
        pt = cell_func(cell_ref(hash_quote), cell_list(pt, NIL));
        return binary(pt, lv, in);

    case it_LPAR:
        // read as list
        pt = getlist(it, it_COMMA, it_RPAR, l_BASE, in); // no colon allowed
        it = lexical(in);
        if (cell_is_list(pt)) {
            // single item on list, not sure what it is
            if (!it || it->type != it_LBRC) {
                // not a function definition
                cell *p2 = pt->_.cons.car; // pick 1st item on list
                pt->_.cons.car = 0;
		cell_unref(pt);
                if (it) pushitem(it);
                return binary(p2, lv, in);
            }
        } else {
            // must be an anomymous function defintion
            if (!it || it->type != it_LBRC) {
		error_par(lxfile_info(in), "expected function body (left curly bracket)");
                if (it) pushitem(it);
                // assume empty function body
                return cell_func(cell_ref(hash_lambda), cell_list(pt, NIL));
            }
        }
        // get function body
        {
            cell *body = getlist(it, it_SEMI, it_RBRC, l_BASE, in);
            pt = cell_func(cell_ref(hash_lambda), cell_list(pt, body));
        }
        return binary(pt, lv, in);

    case it_LBRC: // assoc definition or a compound statement
        pt = getlist(it, it_COMMA, it_RBRC, l_PERHAPS, in);
        // TODO implement optional final semicolon
        pt = cell_func(cell_ref(hash_assoc), pt);
        return binary(pt, lv, in);

    case it_LBRK: // list or vector definition
        pt = getlist(it, it_COMMA, it_RBRK, l_LABEL, in);
        pt = cell_func(cell_ref(hash_list), pt);
        return binary(pt, lv, in);

    case it_RANGE: // start of range is empty, treat as binary
        pushitem(it);
        return binary(NIL, lv, in);

    case it_SEMI:
        if (lv < l_SEMI) {
            // TODO sure???
	    error_pat(lxfile_info(in), "misplaced semicolon", it->type);
        }
        dropitem(it);
        return expr(lv, in);

    case it_RPAR: // cannot be used
    case it_RBRK:
    case it_RBRC:
        // TODO lv > l_SEMI do something, insert void dummy instead

    case it_PLUS: // unary?
    case it_MULT: // binary only
    case it_DIV:
    case it_LT:
    case it_GT:
    case it_EQ:
    case it_LTEQ:
    case it_GTEQ:
    case it_NTEQ:
    case it_EQEQ:
    case it_AMP:
    case it_CAT:
    case it_BAR:
    case it_CIRC:
    case it_AND:
    case it_OR:
    case it_STOP:
    case it_COMMA:
    case it_QUEST: // ternary
    case it_COLON:
        error_pat(lxfile_info(in), "misplaced item, ignored", it->type);
        dropitem(it);
        return expr(lv, in);

    default:
        assert(0);
    }

    return 0;
}

static cell *binary_l2rN(cell *left, precedence l2, cell *func, item *op, precedence lv, lxfile *in) {
    cell *args = NIL;
    cell **next = &args;
    cell *right;
    token type0 = op->type;

    if (lv >= l2) { // left-to-right
        // look no further
        cell_unref(func); // TODO inefficient
        pushitem(op);
        return left;
    }
    do {
        dropitem(op);
        right = expr(l2, in);
        if (!right) {
            badeof(); // end of file
            return cell_func(func, cell_list(left, args));
        }
        *next = cell_list(right, NIL);
        next = &((*next)->_.cons.cdr);
        op = lexical(in);
    } while (op->type == type0);
    pushitem(op);

    return binary(cell_func(func, cell_list(left, args)), lv, in);
}

static cell *binary_l2r(cell *left, precedence l2, cell *func, item *op, precedence lv, lxfile *in) {
    cell *right;

    if (lv >= l2) { // left-to-right
        // look no further
        cell_unref(func); // TODO inefficient
        pushitem(op);
        return left;
    }
    dropitem(op);
    right = expr(l2, in);
    if (!right) {
        badeof(); // end of file
        return cell_func(func, cell_list(left, NIL));
    }
    return binary(cell_func(func, cell_list(left, cell_list(right, NIL))), lv, in);
}

static cell *binary_r2l(cell *left, precedence l2, cell *func, item *op, precedence lv, lxfile *in) {
    cell *right;

    if (lv > l2) { // right-to-left
        // look no further
        cell_unref(func); // TODO inefficient
        pushitem(op);
        return left;
    }
    dropitem(op);
    right = expr(l2, in);
    if (!right) {
        badeof(); // end of file
        return cell_func(func, cell_list(left, NIL));
    }
    return binary(cell_func(func, cell_list(left, cell_list(right, NIL))), lv, in);
}

static cell *binary(cell *left, precedence lv, lxfile *in) {
    item *op = lexical(in);
    if (!op) return left; // end of file

    switch (op->type) {
    case it_PLUS: // binary
	return binary_l2rN(left, l_ADD,  cell_ref(hash_plus), op, lv, in);

    case it_MINUS:
	return binary_l2rN(left, l_ADD,  cell_ref(hash_minus), op, lv, in);

    case it_MULT:
        return binary_l2rN(left, l_MULT, cell_ref(hash_times), op, lv, in);

    case it_DIV:
	left = binary_l2rN(left, l_MULT, cell_ref(hash_quotient), op, lv, in);
	if (cell_is_func(left) && cell_car(left) == hash_quotient
	 && cell_cdr(left) && cell_is_integer(cell_car(cell_cdr(left)))
	 && cell_cdr(cell_cdr(left)) && cell_is_integer(cell_car(cell_cdr(cell_cdr(left))))) {
	    // convert into quotient constant
	    number nval;
	    cell *c;
            nval.dividend.ival = cell_car(cell_cdr(left))->_.n.dividend.ival;
            nval.divisor = cell_car(cell_cdr(cell_cdr(left)))->_.n.dividend.ival;
            if (!normalize_q(&nval)) {
                return err_overflow(left);
            }
            if ((c = cell_cdr(cell_cdr(cell_cdr(left))))) { // more than 2 args?
                c = cell_func(cell_ref(hash_quotient),
                              cell_list(cell_number(&nval), cell_ref(c)));
            } else {
                c = cell_number(&nval);
            }
            cell_unref(left);
            left = c;
	}
	return left;

    case it_LT:
        return binary_l2rN(left, l_REL,  cell_ref(hash_lt), op, lv, in);

    case it_GT:
        return binary_l2rN(left, l_REL,  cell_ref(hash_lt), op, lv, in);

    case it_LTEQ:
        return binary_l2rN(left, l_REL,  cell_ref(hash_le), op, lv, in);

    case it_GTEQ:
        return binary_l2rN(left, l_REL,  cell_ref(hash_ge), op, lv, in);

    case it_NTEQ:
        return binary_l2rN(left, l_EQEQ, cell_ref(hash_noteq), op, lv, in);

    case it_EQEQ:
        return binary_l2rN(left, l_EQEQ, cell_ref(hash_eq), op, lv, in);

    case it_CAT:
        return binary_l2rN(left, l_CAT,  cell_ref(hash_cat), op, lv, in);

    case it_AMP:
        return binary_l2rN(left, l_AMP,  cell_symbol("#bitand"), op, lv, in);

    case it_BAR:
        return binary_l2rN(left, l_BAR,  cell_symbol("#bitor"), op, lv, in);

    case it_CIRC:
        return binary_l2rN(left, l_BAR,  cell_symbol("#bitxor"), op, lv, in); // TODO l_BAR?

    case it_AND:
        return binary_l2rN(left, l_AND,  cell_ref(hash_and), op, lv, in);

    case it_OR:
	return binary_l2rN(left, l_OR,   cell_ref(hash_or), op, lv, in);

    case it_STOP:
        return binary_l2r(left, l_STOP,  cell_ref(hash_refq), op, lv, in); // 2 args

    case it_EQ:
        return binary_r2l(left, l_DEF,   cell_ref(hash_defq), op, lv, in); // 2 args

    case it_QUEST: // ternary
        if (lv > l_COND) { // right-to-left
            // look no further
            pushitem(op);
            return left;
        }
        dropitem(op);
	{
            cell *right = expr(l_COND, in);
	    if (!right) {
		badeof(); // end of file
		return left;
	    }
	    op = lexical(in);
	    if (!op || op->type != it_COLON) {
		// "if" without an "else"
		if (op) pushitem(op);
		return binary(cell_func(cell_ref(hash_if), cell_list(left, cell_list(right, NIL))), lv, in);
	    }
	    dropitem(op);
	    {
		cell *third;
		third = expr(l_COND, in);
		if (!third) {
		    badeof(); // end of file
		    return binary(cell_func(cell_ref(hash_if), cell_list(left, cell_list(right, NIL))), lv, in);
		}
		return binary(cell_func(cell_ref(hash_if), cell_list(left, cell_list(right, cell_list(third, NIL)))), lv, in);
	    }
	}

    case it_LPAR: // function invocation
	if (lv >= l_POST) { // left-to-right
            // look no further
            pushitem(op);
            return left;
        }
        return binary(cell_func(left, getlist(op, it_COMMA, it_RPAR, l_LABEL, in)), lv, in);

    case it_LBRK: // array/assoc index
	if (lv >= l_POST) { // left-to-right
            // look no further
            pushitem(op);
            return left;
        }
        dropitem(op);
        // TODO argument list, not expression
        {
            cell *right = expr(l_BASE, in);
            if (!right) {
                badeof(); // end of file
                return left;
            }
            op = lexical(in);
            if (op->type == it_RBRK) {
                dropitem(op);
            } else {
                error_par(lxfile_info(in), "expected matching right bracket for array");
                if (op) pushitem(op);
            }
            return binary(cell_func(cell_ref(hash_ref), cell_list(left, cell_list(right, NIL))), lv, in);
        }

    case it_COLON:
        if (lv > l_LABEL) { // right-to-left
            // look no further
            pushitem(op);
            return left;
        }
        dropitem(op);
        {
            cell *right = expr(l_LABEL, in);
            if (!right) {
                badeof(); // end of file
                return left;
            }
            return binary(cell_label(left, right), lv, in);
        }

    case it_RANGE:
        // TODO range: should only recognize within brackets?
        if (lv >= l_RANGE) { // left-to-right
            pushitem(op); // look no further
            return left;
        }
        dropitem(op);
        // TODO range: ugly hack for it_RBRK
        op = lexical(in);
        if (!op) {
            badeof(); // end of file
            return cell_range(left, NIL);
        }
        if (op->type == it_RBRK) {
            pushitem(op);
            return binary(cell_range(left, NIL), lv, in);
        } else {
            cell *right;
            pushitem(op);
            if (!(right = expr(l_RANGE, in))) {
                badeof(); // end of file
                return cell_range(left, NIL);
            }
            return binary(cell_range(left, right), lv, in);
        }

    case it_SEMI:
#if 0 // TODO review for later
        if (lv >= l_SEMI) { // TODO left-to-right?
            // look no further
            pushitem(op);
            return left;
        }
        dropitem(op);
        right = expr(lv, in);
        if (!right) {
            badeof(); // end of file
            return left;
        }
        // TODO as with '+' and '*' and '>' etc, catenate
        // TODO we do not really need the #do, do we?
        // return binary(cell_func(cell_ref(hash_do), cell_list(left, cell_list(right, NIL))), lv, in);
        // return binary(cell_list(left, cell_list(right, NIL)), lv, in);
        // this is a lambda with empty argument list
        return cell_func(cell_ref(hash_lambda), cell_list(NIL, cell_list(left, cell_list(right, NIL))));
#endif
    case it_RPAR:
    case it_RBRK:
    case it_RBRC:
    case it_COMMA:
        // parse no more
        pushitem(op);
        return left;

    case it_NOT: // unary only
    case it_QUOTE:
    case it_TILDE:
    case it_LBRC:
    case it_NUMBER:
    case it_STRING:
    case it_SYMBOL:
        break;
    default:
	error_pat(lxfile_info(in), "ASSERT operator", op->type); // TODO
        assert(0);
    }
    error_pat(lxfile_info(in), "misplaced operator", op->type);
    dropitem(op);
    return left;
}


//
// parse list of function arguments or parameters
//
static cell *getlist(item *op, token sep_token, token end_token, 
                     precedence blv, lxfile *in) {
    cell *arglist = 0;
    cell **nextp = &arglist;
    cell *arg;

    // assume left parenthesis is current
    dropitem(op);
    op = lexical(in);
    if (!op) {
        badeof();
        return arglist;
    }
    if (op->type == end_token) {
        // special case: zero arguments
        dropitem(op);
        return arglist;
    }
    pushitem(op);
    for (;;) {
        arg = expr(blv, in);          // TODO what about comma?
        if (!arg) {
            badeof(); // end of file
            return arglist;
        }
        if (blv == l_PERHAPS) { // ugly curly brace case
            // use first item to decide what this is
            if (cell_is_label(arg)) blv = l_LABEL;
            else blv = l_BASE;
        }
        *nextp = cell_list(arg, NIL);
        nextp = &((*nextp)->_.cons.cdr);
        op = lexical(in);
        if (!op) {
            badeof();
            return arglist;
        }
        if (op->type != sep_token) break;
        dropitem(op);
    }
    if (op->type != end_token) {
	error_par(lxfile_info(in), "expected matching right parenthesis");
        pushitem(op);
        return arglist;
    }
    dropitem(op);
    return arglist;
}
