/*  TAB-P
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cell.h"
#include "compile.h"
#include "node.h"
#include "cmod.h"
#include "qfun.h"
#include "opt.h"
#include "err.h"

struct compile_env {
    struct compile_env *prev;
    cell *vars;
    int params;
    int locals;
    int ref_local;
    int ref_closure;
    int ref_global;
} ;

static int compile1(cell *prog, cell ***nextpp, struct compile_env *cep);
static void compile1void(cell *prog, cell ***nextpp, struct compile_env *cep);
static cell *compile_prog(cell *prog, struct compile_env *cep);

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
static int compile_args(cell *args, cell ***nextpp, struct compile_env *cep) {
    int n = 0;
    cell *arg;
    if (list_pop(&args, &arg)) {
        n = compile_args(args, nextpp, cep);
        n += compile1(arg, nextpp, cep);
    }
    return n;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_if(cell *args, cell ***nextpp, struct compile_env *cep) {
    cell *cond;
    cell *iftrue;
    cell *iffalse;
    if (!list_pop(&args, &cond)) {
        cell_unref(error_rt0("missing condition for if"));
        return 0; // empty #if
    }
    compile1void(cond, nextpp, cep);
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
        compile1void(iffalse, nextpp, cep);
        condp = &((*condp)->_.cons.car); // iftrue path
        assert(*condp == NIL);
        compile1void(iftrue, &condp, cep);
        noop = add2prog(c_DONOOP, NIL, nextpp); // TODO can be improved
        *condp = cell_ref(noop); // join up
    }
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_and(cell *args, cell ***nextpp, struct compile_env *cep) {
    cell *test;
    cell *pushf = NIL;
    cell **pushfp = &pushf;

    while (list_pop(&args, &test)) {
        compile1void(test, nextpp, cep);
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
static int compile1_or(cell *args, cell ***nextpp, struct compile_env *cep) {
    cell *test;
    cell *pusht = NIL;
    cell **pushtp = &pusht;

    while (list_pop(&args, &test)) {
        compile1void(test, nextpp, cep);
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
static int compile1_defq(cell *args, cell ***nextpp, struct compile_env *cep) {
    cell *nam;
    cell *val;

    if (!arg2(args, &nam, &val)) {
        return 0; // error
    }

    if (cep) {
        if (!assoc_set(cep->vars, cell_ref(nam), cell_ref(val))) {
            cell_unref(error_rt1("cannot redefine local value", nam));
            cell_unref(val);
            return 0;
        } else {
            ++(cep->locals);
        }
    } else {
        // TODO do what if define is global?
    }

    compile1void(val, nextpp, cep);
    add2prog(c_DODEFQ, nam, nextpp);

    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_refq(cell *args, cell ***nextpp, struct compile_env *cep) {
    cell *nam;
    cell *val;

    if (!arg2(args, &val, &nam)) {
        return 0; // error
    }

    compile1void(val, nextpp, cep);
    add2prog(c_DOREFQ, nam, nextpp);
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_lambda(cell *args, cell ***nextpp, struct compile_env *cep) {
    cell *prog;
    cell *paramlist;
    cell *cp;
    if (!list_pop(&args, &paramlist)) {
        cell_unref(error_rt1("Missing function parameter list", args));
            return 0;
    }

    // assign a new level of compile environment
    {
        struct compile_env *newenv = malloc(sizeof(struct compile_env));
        assert(newenv);
        memset(newenv, 0, sizeof(struct compile_env));
        newenv->prev = cep;
        newenv->vars = cell_assoc();
        cep = newenv;
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
            // TODO compile/evaluate default value when?
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
        } else if (!assoc_set(cep->vars, cell_ref(param), NIL)) { // TODO smarter value than NIL
            // TODO what to do about this?
            cell_unref(error_rt1("duplicate parameter name", param));
        } else {
            ++(cep->params);
        }

        cp = cell_cdr(cp);
    }

    // now compile the function body
    prog = compile_prog(args, cep);

    cp = cell_lambda(paramlist, prog);

    // closure dealt with at runtime
    add2prog(c_DOLAMB, cp, nextpp);

    // drop locals
    {
        struct compile_env *prevenv = cep->prev;
        if (opt_showcode) { // debug?
            printf("\nlambda: params=%d, locals=%d, refs: local=%d, closure=%d, global=%d\n",
                    cep->params, cep->locals, cep->ref_local, cep->ref_closure, cep->ref_global);
        }
        cell_unref(cep->vars);
        free(cep);
        cep = prevenv;
    }

    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_apply(cell *args, cell ***nextpp, struct compile_env *cep) {
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

    compile1void(tailargs, nextpp, cep);
    compile1void(fun, nextpp, cep);
    add2prog(c_DOAPPLY, NIL, nextpp);
    return 1;
}

// compile, substituting void value on error
static void compile1void(cell *prog, cell ***nextpp, struct compile_env *cep) {
    if (!compile1(prog, nextpp, cep)) {
        add2prog(c_DOQPUSH, cell_ref(hash_void), nextpp); // error
    }
}

// return 1 if something was pushed, 0 otherwise
static int compile1(cell *prog, cell ***nextpp, struct compile_env *cep) {

    switch (prog ? prog->type : c_LIST) {
    case c_FUNC: // function call
        {
            cell *fun = cell_ref(prog->_.cons.car); // TODO make split function for this?
            cell *args = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);

            // TODO instead of this, if symbol, evaluate it here and decide what to do

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
                return compile1_defq(args, nextpp, cep);

            } else if (fun == hash_refq) { // #refq is special case
                cell_unref(fun);
                return compile1_refq(args, nextpp, cep);

            } else if (fun == hash_if) { // #if is special case
                cell_unref(fun);
                return compile1_if(args, nextpp, cep);

            } else if (fun == hash_and) { // #and is special case
                cell_unref(fun);
                return compile1_and(args, nextpp, cep);

            } else if (fun == hash_or) { // #or is special case
                cell_unref(fun);
                return compile1_or(args, nextpp, cep);

            } else if (fun == hash_lambda) { // #lambda is special case
                cell_unref(fun);
                return compile1_lambda(args, nextpp, cep);

            } else if (fun == hash_apply) { // #apply is special case
                cell_unref(fun);
                return compile1_apply(args, nextpp, cep);

            } else {
		cell *def = NIL;
                int n = compile_args(args, nextpp, cep);
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
                    if (!compile1(fun, nextpp, cep)) {
                        add2prog(c_DOQPUSH, cell_ref(hash_void), nextpp); // error
                    }
                    node = add2prog(t, def, nextpp);
                    node->_.calln.narg = n;
                    assert(*nextpp == &(node->_.calln.cdr)); // TODO assumption
                } else {
                    // see if known internal symbol
                    // TODO when compiler does local variables at compile time, can optimize much more
                    // TODO can also omptimize other cases when known, e.g. c_CLOSURE etc
                    if (cell_is_symbol(fun) && fun->_.symbol.nam && fun->_.symbol.nam[0]=='#') {
                        // built in known global
                        def = cell_ref(fun->_.symbol.val);
                        cell_unref(fun);
                    } else {
                        // compile, pushing on stack
                        if (!compile1(fun, nextpp, cep)) {
                            add2prog(c_DOQPUSH, cell_ref(hash_void), nextpp); // error
                        }
                    }
                    add2prog(t, def, nextpp);
                }

                return 1;
            }
        }

    case c_SYMBOL:
        // look for non-global definition
        if (cep) {
            ++(cep->ref_global);
            struct compile_env *e = cep;
            do {
                cell *val;
                if (assoc_get(e->vars, prog, &val)) {
                    // TODO: can optimize away if pure value
                    if (e == cep) {
                        ++(cep->ref_local);
                    } else {
                        ++(cep->ref_closure);
                    }
                    --(cep->ref_global);
                    cell_unref(val);
                    break;
                }
                // levels above
                e = e->prev;
            } while (e);
        }

        add2prog(c_DOEPUSH, prog, nextpp);
        return 1;

    case c_LABEL: // car is label, cdr is expr
        {
            cell *label;
            cell *val;
            label_split(prog, &label, &val);
            compile1void(val, nextpp, cep);
            if (cell_is_symbol(label) || cell_is_number(label)) {
                add2prog(c_DOLABEL, label, nextpp);
            } else {
                compile1void(label, nextpp, cep);
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
                compile1void(upper, nextpp, cep);
            }
            if (lower == NIL) {
                add2prog(c_DOQPUSH, NIL, nextpp);
            } else {
                compile1void(lower, nextpp, cep);
            }
            add2prog(c_DORANGE, NIL, nextpp);
        }
        return 1;

    case c_STRING:
    case c_NUMBER:
    case c_SPECIAL:
    case c_CLOSURE:
    case c_CLOSURE0:
    case c_CLOSURE0T:
        add2prog(c_DOQPUSH, prog, nextpp);
        return 1;

    default:
        cell_unref(error_rt1("cannot compile", prog));
        return 0;
    }
}

// compile list of expressions
static cell *compile_prog(cell *prog, struct compile_env *cep) {
    cell *item;
    int stacklevel = 0;
    cell *result = NIL;
    cell **nextp = &result;
    while (list_pop(&prog, &item)) {
        if (stacklevel > 0) {
            add2prog(c_DOPOP, NIL, &nextp); // TODO inefficient
            --stacklevel;
        }
        if (compile1(item, &nextp, cep)) ++stacklevel;
    }
    // TODO what about stacklevel?
    return result; 
}

// if total failure, result is NIL
// TODO deal with that
cell *compile(cell *prog) {
    cell *result = NIL;
    cell **nextp = &result;
    compile1(prog, &nextp, NULL);
    return result; 
}



