/*  TAB-P
 *
 *
 */

#include <assert.h>

#include "cell.h"
#include "compile.h"
#include "node.h"
#include "cmod.h"
#include "qfun.h"
#include "err.h"

static int compile1(cell *prog, cell ***nextpp);
static void compile1void(cell *prog, cell ***nextpp);
static cell *compile_list(cell *prog);

// return unreffed node if needed
static cell *add2prog(enum cell_t type, cell *car, cell ***nextpp) {
    cell *node = newnode(type);
    node->_.cons.car = car;
    node->_.cons.cdr = NIL;
    **nextpp = node;
    (*nextpp) = &(node->_.cons.cdr);
    return node;
}

// compile and push arguments last to first, return count
static int compile_args(cell *args, cell ***nextpp) {
    int n = 0;
    cell *arg;
    if (list_pop(&args, &arg)) {
        n = compile_args(args, nextpp);
        n += compile1(arg, nextpp);
    }
    return n;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_if(cell *args, cell ***nextpp) {
    cell *cond;
    cell *iftrue;
    cell *iffalse;
    if (!list_pop(&args, &cond)) {
        cell_unref(error_rt0("missing condition for if"));
        return 0; // empty #if
    }
    compile1void(cond, nextpp);
    if (!list_pop(&args, &iftrue)) {
        cell_unref(error_rt0("missing value if true for if"));
        return 1;
    }
    if (list_pop(&args, &iffalse)) {
        arg0(args); // no more
    } else {
        iffalse = cell_ref(hash_void); // default to void value for else
    }
    {
        cell **condp = *nextpp;
        cell *noop;
        add2prog(c_DOCOND, NIL, nextpp); // have NIL dummy for iftrue
        compile1void(iffalse, nextpp);
        condp = &((*condp)->_.cons.car); // iftrue path
        assert(*condp == NIL);
        compile1void(iftrue, &condp);
        noop = add2prog(c_DONOOP, NIL, nextpp); // TODO can be improved
        *condp = cell_ref(noop); // join up
    }
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_and(cell *args, cell ***nextpp) {
    cell *test;
    cell *pushf = NIL;
    cell **pushfp = &pushf;

    while (list_pop(&args, &test)) {
        compile1void(test, nextpp);
        if (args == NIL) { // last item does not need to be read
            break;
        }
        // more items, so need a "push false"
        if (pushf == NIL) {
            add2prog(c_DOQPUSH, cell_ref(hash_f), &pushfp); // make NIL dummy for iffalse
        }
        {
            cell *testp = add2prog(c_DOCOND, NIL, nextpp); // have NIL dummy for iftrue, filled in below
            **nextpp = cell_ref(pushf); // iffalse, go to push false
            *nextpp = &(testp->_.cons.car); // iftrue, continue
        }
    }
    if (pushf != NIL) {
        // patch up missing links
        cell *noop = add2prog(c_DONOOP, NIL, nextpp); // TODO can be improved
        *pushfp = cell_ref(noop); // join up
        cell_unref(pushf);
    }
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_or(cell *args, cell ***nextpp) {
    cell *test;
    cell *pusht = NIL;
    cell **pushtp = &pusht;

    while (list_pop(&args, &test)) {
        compile1void(test, nextpp);
        if (args == NIL) { // last item does not need to be read
            break;
        }
        // more items, so need a "push true"
        if (pusht == NIL) {
            add2prog(c_DOQPUSH, cell_ref(hash_t), &pushtp); // make NIL dummy for iftrue
        }
        add2prog(c_DOCOND, cell_ref(pusht), nextpp); // go push true iftrue
    }
    if (pusht != NIL) {
        // patch up missing links
        cell *noop = add2prog(c_DONOOP, NIL, nextpp); // TODO can be improved
        *pushtp = cell_ref(noop); // join up
        cell_unref(pusht);
    }
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_defq(cell *args, cell ***nextpp) {
    cell *nam;
    cell *val;

    if (!arg2(args, &nam, &val)) {
        return 0; // error
    }
    compile1void(val, nextpp);
    add2prog(c_DODEFQ, nam, nextpp);
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_refq(cell *args, cell ***nextpp) {
    cell *nam;
    cell *val;

    if (!arg2(args, &val, &nam)) {
        return 0; // error
    }
    compile1void(val, nextpp);
    add2prog(c_DOREFQ, nam, nextpp);
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_lambda(cell *args, cell ***nextpp) {
    cell *prog;
    cell *paramlist;
    cell *cp;
    if (!list_pop(&args, &paramlist)) {
        cell_unref(error_rt1("Missing function parameter list", args));
            return 0;
    }
    // TODO consider moving to parser
    // check for proper and for duplicate parameters
    cp = paramlist;
    while (cp) {
        cell *param;
        assert(cell_is_list(cp));
        param = cell_car(cp);
        if (cell_is_label(param)) {
            param = param->_.cons.car;
            // TODO evaluate default value
        }
        if (param == hash_ellip) {
            // TODO what to do about this?
            if (cell_cdr(cp)) {
                cell_unref(error_rt1("ellipsis must be last parameter", cp));
                cp = NIL;
            }
        } else if (!cell_is_symbol(param)) {
            // TODO what to do about this?
            cell_unref(error_rt1("parameter not a symbol", cell_ref(param)));
        } else if (exists_on_list(cell_cdr(cp), param)) {
            // TODO what to do about this?
            cell_unref(error_rt1("duplicate parameter name", cell_ref(param)));
        }
        cp = cell_cdr(cp);
    }

    // now compile the function body
    prog = compile_list(args);

    cp = cell_lambda(paramlist, prog);

    // closure dealt with at runtime
    add2prog(c_DOLAMB, cp, nextpp);
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_apply(cell *args, cell ***nextpp) {
    cell *fun;
    cell *tailargs;
    if (!list_pop(&args, &fun)) {
        cell_unref(error_rt1("missing function", args));
        return 0;
    }
    // TODO can have intermediate args, or perhaps even zero args?
    if (!list_pop(&args, &tailargs)) {
        cell_unref(fun);
        cell_unref(error_rt1("missing tail arguments", args));
        return 0;
    }
    arg0(args);

    compile1void(tailargs, nextpp);
    compile1void(fun, nextpp);
    add2prog(c_DOAPPLY, NIL, nextpp);
    return 1;
}

// compile, substituting void value on error
static void compile1void(cell *prog, cell ***nextpp) {
    if (!compile1(prog, nextpp)) {
        add2prog(c_DOQPUSH, cell_ref(hash_void), nextpp); // error
    }
}

// return 1 if something was pushed, 0 otherwise
static int compile1(cell *prog, cell ***nextpp) {

    switch (prog ? prog->type : c_LIST) {
    case c_FUNC:
        {
            cell *fun = cell_ref(prog->_.cons.car); // TODO make split function for this?
            cell *args = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);

            if (fun == hash_quote) { // #quote is special case
                cell *thing;
                cell_unref(fun);
                if (!arg1(args, &thing)) {
                    return 0;
                }
                add2prog(c_DOQPUSH, thing, nextpp);
                return 1;

            } else if (fun == hash_defq) { // #defq is special case
                cell_unref(fun);
                return compile1_defq(args, nextpp);

            } else if (fun == hash_refq) { // #refq is special case
                cell_unref(fun);
                return compile1_refq(args, nextpp);

            } else if (fun == hash_if) { // #if is special case
                cell_unref(fun);
                return compile1_if(args, nextpp);

            } else if (fun == hash_and) { // #and is special case
                cell_unref(fun);
                return compile1_and(args, nextpp);

            } else if (fun == hash_or) { // #or is special case
                cell_unref(fun);
                return compile1_or(args, nextpp);

            } else if (fun == hash_lambda) { // #lambda is special case
                cell_unref(fun);
                return compile1_lambda(args, nextpp);

            } else if (fun == hash_apply) { // #apply is special case
                cell_unref(fun);
                return compile1_apply(args, nextpp);

            } else {
		cell *def = NIL;
                int n = compile_args(args, nextpp);
                enum cell_t t;

                switch (n) {
                case 1:
                    t = c_DOCALL1;
                    break;
                case 2:
                    t = c_DOCALL2;
                    break;
                default:
                    t = c_DOCALLN;
                    break;
                }
                if (t == c_DOCALLN) {
                    cell *node;
                    // compile, pushing on stack
                    if (!compile1(fun, nextpp)) {
                        add2prog(c_DOQPUSH, cell_ref(hash_void), nextpp); // error
                    }
                    node = add2prog(t, def, nextpp);
                    node->_.calln.narg = n;
                    assert(*nextpp == &(node->_.calln.cdr)); // TODO assumption
                } else {
                    // see if known internal symbol
                    // TODO when compiler does local variables at compile time, can optimize much more
                    if (cell_is_symbol(fun) && fun->_.symbol.nam && fun->_.symbol.nam[0]=='#') {
                        // built in known global
                        def = cell_ref(fun->_.symbol.val);
                        cell_unref(fun);
                    } else {
                        // compile, pushing on stack
                        if (!compile1(fun, nextpp)) {
                            add2prog(c_DOQPUSH, cell_ref(hash_void), nextpp); // error
                        }
                    }
                    add2prog(t, def, nextpp);
                }

                return 1;
            }
        }

    case c_SYMBOL:
        add2prog(c_DOEPUSH, prog, nextpp);
        return 1;

    case c_LABEL: // car is label, cdr is expr
        {
            cell *label;
            cell *val;
            label_split(prog, &label, &val);
            compile1void(val, nextpp);
            if (cell_is_symbol(label) || cell_is_number(label)) {
                add2prog(c_DOLABEL, label, nextpp);
            } else {
                compile1void(label, nextpp);
                add2prog(c_DOLABEL, NIL, nextpp);
            }
        }
        return 1;

    case c_RANGE: // car is lower, cdr is upper bound; both may be NIL
        {
            cell *lower;
            cell *upper;
            range_split(prog, &lower, &upper);
            // TODO assume nam is quoted: cell_is_symbol(nam) cell_is_number(nam)
            if (upper == NIL) {
                add2prog(c_DOQPUSH, NIL, nextpp); // TODO can optimize..
            } else {
                compile1void(upper, nextpp);
            }
            if (lower == NIL) {
                add2prog(c_DOQPUSH, NIL, nextpp);
            } else {
                compile1void(lower, nextpp);
            }
            add2prog(c_DORANGE, NIL, nextpp);
        }
        return 1;

    case c_STRING:
    case c_NUMBER:
        add2prog(c_DOQPUSH, prog, nextpp);
        return 1;

    default:
        cell_unref(error_rt1("cannot compile", prog));
        return 0;
    }
}

static cell *compile_list(cell *prog) {
    cell *item;
    int stacklevel = 0;
    cell *result = NIL;
    cell **nextp = &result;
    while (list_pop(&prog, &item)) {
        if (stacklevel > 0) {
            add2prog(c_DOPOP, NIL, &nextp); // TODO inefficient
            --stacklevel;
        }
        if (compile1(item, &nextp)) ++stacklevel;
    }
    // TODO what about stacklevel?
    return result; 
}

// if total failure, result is NIL
// TODO deal with that
cell *compile(cell *prog) {
    cell *result = NIL;
    cell **nextp = &result;
    compile1(prog, &nextp);
    return result; 
}



