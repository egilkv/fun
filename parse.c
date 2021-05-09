/* TAB-P
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lex.h"
#include "parse.h"
#include "cfun.h"
#include "err.h"

static cell *expr(precedence lv);
static cell *getlist(item *op, token sep_token, token end_token);
static cell *binary(cell *left, precedence lv);

static cell *badeof() {
    error_par("unexpected end of file");
    return 0;
}

cell *expression() {
    return expr(l_BOT);
}

static cell *expr(precedence lv) {
    cell *pt = 0;
    cell *p2 = 0;
    item *it;
    it = lexical();
    if (!it) return 0; // end of file

    switch (it->type) {
    case it_SEMI: // special
        dropitem(it);
        return expr(lv); // TODO end recursion in C is a problem

    case it_INTEGER:
        pt = cell_integer(it->ivalue);
        dropitem(it);
        return binary(pt, lv);

    case it_STRING:
        pt = cell_astring(it->svalue);
        it->svalue = 0;
        dropitem(it);
        return binary(pt, lv);

    case it_SYMBOL:
        pt = cell_asymbol(it->svalue);
        it->svalue = 0;
        dropitem(it);
        return binary(pt, lv);

    case it_NOT: // unary only
        dropitem(it);
        p2 = expr(l_UNARY);
        if (!p2) return badeof();
	return cell_cons(cell_ref(hash_not), cell_cons(p2, NIL));

    case it_MINS:
        dropitem(it);
        p2 = expr(l_UNARY);
        if (!p2) return badeof();
	return cell_cons(cell_ref(hash_minus), cell_cons(p2, NIL));

    case it_QUOT:
        dropitem(it);
        p2 = expr(l_UNARY);
        if (!p2) return badeof();
	return cell_cons(cell_ref(hash_quote), cell_cons(p2, NIL));

    case it_LPAR:
        // read as list
        pt = getlist(it, it_COMA, it_RPAR);
        it = lexical();
        if (pt && pt->type == c_CONS && !pt->_.cons.cdr) {
            // single item on list, not sure what it is
            if (!it || it->type != it_LBRC) {
                // not a function definition
                cell *p2 = pt->_.cons.car; // pick 1st item on list
                pt->_.cons.car = 0;
		cell_unref(pt);
                if (it) pushitem(it);
                return binary(p2, lv);
            }
        } else {
            // must be an anomymous function defintion
            if (!it || it->type != it_LBRC) {
		error_par("expected function body (left curly bracket)");
                if (it) pushitem(it);
                // assume empty function body
                return cell_cons(cell_ref(hash_lambda), cell_cons(pt, NIL));
            }
        }
        {
            cell *body = getlist(it, it_SEMI, it_RBRC);
            return cell_cons(cell_ref(hash_lambda), cell_cons(pt, body));
        }

    case it_LBRC:
        pt = getlist(it, it_SEMI, it_RBRC);
        // TODO implement optional final semicolon
        return cell_cons(cell_symbol("#do"), pt);

    case it_LBRK:
        // TODO ellipsis cannot be treated exactly as comma
        pt = getlist(it, it_ELIP, it_RBRK);
        it = lexical();
        if (!it || it->type != it_LBRC) {
	    error_par("expected array initializer (left curly bracket)");
            if (it) pushitem(it);
            // assume empty initializer
            return cell_cons(cell_ref(hash_vector), cell_cons(pt, NIL));
        }
        {
            cell *init = getlist(it, it_COMA, it_RBRC);
	    // TODO vector of straight length is special case
            if (cell_is_cons(pt) && cell_cdr(pt) == NIL) {
		// have number, not a list
		cell_split(pt, &pt, NULL);
	    }
            return cell_cons(cell_ref(hash_vector), cell_cons(pt, init));
        }

    case it_PLUS: // unary?
    case it_MULT: // binary only
    case it_DIVS:
    case it_LT:
    case it_GT:
    case it_EQUL:
    case it_LTEQ:
    case it_GTEQ:
    case it_NTEQ:
    case it_EQEQ:
    case it_AMP:
    case it_AND:
    case it_STOP:
    case it_COMA:
    case it_QEST: // ternary
    case it_ELIP: // binary sometimes..
    case it_RPAR: // cannot be used
    case it_RBRK:
    case it_RBRC:
	error_pat("misplaced item, syntax error", it->type);
        dropitem(it);
        return expr(lv);

    default:
        assert(0);
    }

    return 0;
}

static cell *binary(cell *left, precedence lv) {
    precedence l2 = l_BOT;
    cell *s = 0;
    cell *right = 0;
    item *op = lexical();
    if (!op) return left; // end of file

    switch (op->type) {
    case it_PLUS: // binary
	if (!s) { l2 = l_ADD;     s = cell_ref(hash_plus); }
    case it_MINS:
	if (!s) { l2 = l_ADD;     s = cell_ref(hash_minus); }
    case it_MULT:
	if (!s) { l2 = l_MULT;    s = cell_ref(hash_times); }
    case it_DIVS:
	if (!s) { l2 = l_MULT;    s = cell_ref(hash_div); }
    case it_LT:
        if (!s) { l2 = l_REL;     s = cell_ref(hash_lt); }
    case it_GT:
        if (!s) { l2 = l_REL;     s = cell_symbol("#gt"); }
    case it_LTEQ:
        if (!s) { l2 = l_REL;     s = cell_symbol("#lteq"); }
    case it_GTEQ:
        if (!s) { l2 = l_REL;     s = cell_symbol("#gteq"); }
    case it_NTEQ:
        if (!s) { l2 = l_EQ;      s = cell_symbol("#noteq"); }
    case it_EQEQ:
        if (!s) { l2 = l_EQ;      s = cell_symbol("#eq"); }
    case it_AMP:
        if (!s) { l2 = l_AMP;     s = cell_symbol("#amp"); }
    case it_AND:
        if (!s) { l2 = l_AND;     s = cell_symbol("#and"); }
    case it_STOP:
        if (!s) { l2 = l_POST;    s = cell_symbol("#dot"); }
    case it_EQUL:
	if (!s) { l2 = l_DEF;     s = cell_ref(hash_defq); }

        if (lv >= l2) { // TODO left-to-right
            // look no further
            pushitem(op);
            return left;
        }
        dropitem(op);
        right = expr(l2);
        if (!right) {
            badeof(); // end of file
            return left;
        }
        return binary(cell_cons(s, cell_cons(left, cell_cons(right, NIL))), lv);

    case it_QEST: // ternary
        if (lv >= l_COND) { // TODO right to left
            // look no further
            pushitem(op);
            return left;
        }
        dropitem(op);
        right = expr(l_COND);
        if (!right) {
            badeof(); // end of file
            return left;
        }
        op = lexical();
        if (!op || op->type != it_COLO) {
            // "if" without an "else"
            // TODO should that be allowed?
            if (op) pushitem(op);
            return binary(cell_cons(cell_ref(hash_if), cell_cons(left, cell_cons(right, NIL))), lv);
        }
        dropitem(op);
        {
            cell *third;
            third = expr(l_COND);
            if (!third) {
                badeof(); // end of file
                return binary(cell_cons(cell_ref(hash_if), cell_cons(left, cell_cons(right, NIL))), lv);
            }
            return binary(cell_cons(cell_ref(hash_if), cell_cons(left, cell_cons(right, cell_cons(third, NIL)))), lv);
        }

    case it_LPAR: // function
        if (lv >= l_POST) { // TODO left-to-right
            // look no further
            pushitem(op);
            return left;
        }
        return cell_cons(left, getlist(op, it_COMA, it_RPAR));

    case it_LBRK: // array
        if (lv >= l_POST) { // TODO left-to-right
            // look no further
            pushitem(op);
            return left;
        }
        dropitem(op);
        // TODO argument list, not expression
        right = expr(l_BOT);
        if (!right) {
            badeof(); // end of file
            return left;
        }
        op = lexical();
        if (op->type == it_RBRK) {
            dropitem(op);
        } else {
	    error_par("expected matching right bracket for array");
            if (op) pushitem(op);
        }
        return binary(cell_cons(cell_ref(hash_ref), cell_cons(left, cell_cons(right, NIL))), lv);

    case it_SEMI:
    case it_RPAR:
    case it_RBRK:
    case it_RBRC:
    case it_COLO:
    case it_COMA:
    case it_ELIP:
        // parse no more
        pushitem(op);
        return left;


    case it_NOT: // unary only
    case it_QUOT:
    case it_LBRC:
    case it_INTEGER:
    case it_STRING:
    case it_SYMBOL:
        break;
    default:
	error_pat("ASSERT operator", op->type); // TODO
        assert(0);
    }
    error_pat("misplaced operator, syntax error", op->type);
    dropitem(op);
    return left;
}


//
// parse list of function arguments or parameters
//
static cell *getlist(item *op, token sep_token, token end_token) {
    cell *arglist = 0;
    cell **nextp = &arglist;
    cell *arg;

    // assume left parenthesis is current
    dropitem(op);
    op = lexical();
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
        arg = expr(l_BOT);            // TODO what about comma?
        if (!arg) {
            badeof(); // end of file
            return arglist;
        }
        *nextp = cell_cons(arg, NIL);
        nextp = &((*nextp)->_.cons.cdr);
        op = lexical();
        if (!op) {
            badeof();
            return arglist;
        }
        if (op->type != sep_token) break;
        dropitem(op);
    }
    if (op->type != end_token) {
	error_par("expected matching right parenthesis");
        pushitem(op);
        return arglist;
    }
    dropitem(op);
    return arglist;
}



