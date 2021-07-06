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
#include "number.h"
#include "cmod.h"
#include "cfun.h"
#include "qfun.h"
#include "opt.h"
#include "err.h"
#include "debug.h"

struct compile_env {
    struct compile_env *prev;
    cell *vars;
    int params;
    int locals;
    int ref_local;
    int ref_closure;
    int ref_global;
} ;

struct fixup {
    cell **pp;              // where to fix up link to next cell
    struct fixup *chain;    // added fixups
} ;

static int compile1(cell *item, struct fixup *fix, struct compile_env *cep);
static int compile1nc(cell *item, struct fixup *fix, struct compile_env *cep);
static int compile2constant(cell *item, cell **valp, struct compile_env *cep);
static void compile1void(cell *item, struct fixup *fix, struct compile_env *cep);
static void compilenc1void(cell *item, struct fixup *fix, struct compile_env *cep);
static cell *compile_list(cell *tree, struct compile_env *cep);

// prepare a new level of compile environment
static struct compile_env *compile_env_new(struct compile_env *cep, cell *assoc) {
    struct compile_env *newenv = malloc(sizeof(struct compile_env));
    assert(newenv);
    memset(newenv, 0, sizeof(struct compile_env));
    newenv->prev = cep;
    newenv->vars = assoc;
    return newenv;
}

// drop a level of compile environment
static void compile_env_drop(struct compile_env **cepp) {
    struct compile_env *prevenv = (*cepp)->prev;
    assert(cepp && *cepp);
    prevenv = (*cepp)->prev;
    cell_unref((*cepp)->vars);
    free(*cepp);
    *cepp = prevenv;
}

// add fixup for next fixup
static void addfixup(cell **where, struct fixup *fix) {
    struct fixup *f2 = malloc(sizeof(struct fixup));
    assert(f2 != NULL);
    assert(*where == NIL);
    f2->pp = where;
    f2->chain = fix->chain;
    fix->chain = f2;
}

// fix pointers to (new) node (which is consumed)
static void fixprog(cell *node, struct fixup *fix) {
    struct fixup *f2;
    *(fix->pp) = node;
    fix->pp = &(node->_.cons.cdr);
    while ((f2 = fix->chain)) { // more than one fixup needed?
        *(f2->pp) = cell_ref(node);
        fix->chain = f2->chain;
        free(f2);
    }
}

// return unreffed node, if needed
static cell *add2prog(enum cell_t type, cell *car, struct fixup *fix) {
    cell *node = newnode(type);
    node->_.cons.car = car;
    node->_.cons.cdr = NIL;
    fixprog(node, fix);
    return node;
}

// compile and push arguments last to first, return count
static int compile_args(cell *args, struct fixup *fix, struct compile_env *cep) {
    int n = 0;
    cell *arg;
    if (list_pop(&args, &arg)) {
        n = compile_args(args, fix, cep);
        n += compile1(arg, fix, cep);
    }
    return n;
}

// see if arguments all can be compiled to constants. return count, or -1 if not
static int compile2constant_args(cell *args, cell **valp, struct compile_env *cep) {
    cell *val = NIL;
    int n = 0;
    if (args == NIL) {
        *valp = NIL;
        return 0;
    }
    assert(cell_is_list(args));
    cell *nextvals = NIL;
    cell *arg = cell_car(args);
    cell *nextargs = cell_cdr(args);

    if (!compile2constant(cell_ref(arg), &val, cep)) {
        cell_unref(arg);
        return -1;
    }
    if ((n = compile2constant_args(cell_ref(nextargs), &nextvals, cep)) < 0) {
        cell_unref(nextargs);
        cell_unref(val);
        return -1;
    }
    *valp = cell_list(val, nextvals);
    cell_unref(args);
    return n+1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_if(cell *args, struct fixup *fix, struct compile_env *cep) {
    cell *cond;
    int bool = -1;
    cell *iftrue;
    cell *iffalse;
    if (!list_pop(&args, &cond)) {
        cell_unref(error_rt0("missing condition for if"));
        return 0; // empty #if
    }
    if (compile2constant(cond, &cond, cep) && peek_boolean(cond, &bool)) {
        // test condition is constant
        cell_unref(cond);
    } else {
        compilenc1void(cond, fix, cep);
    }
    if (!list_pop(&args, &iftrue)) {
        if (bool >= 0) {
            add2prog(c_DOQPUSH, cell_boolean(bool), fix);
        }
        // leave value from if-test
        cell_unref(error_rt0("missing value if true for if"));
        return 1;
    }
    if (list_pop(&args, &iffalse)) {
        arg0(args); // no more
    } else {
        iffalse = cell_void(); // default to void value for else
    }
    if (bool == 1) {
        compile1void(iftrue, fix, cep);
        cell_unref(iffalse);
    } else if (bool == 0) {
        compile1void(iffalse, fix, cep);
        cell_unref(iftrue);
    } else {
        cell *doif = add2prog(c_DOIF, NIL, fix); // have NIL dummy for iftrue
        cell *doelse = add2prog(c_DOELSE, NIL, fix); // have NIL dummy for iffalse
        struct fixup branch;
        branch.chain = NULL;

        branch.pp = &(doif->_.cons.car); // iftrue path
        assert(*(branch.pp) == NIL);
        compile1void(iftrue, &branch, cep);
#if 1 // TODO remove alternative
        addfixup(branch.pp, fix);
#else
        assert(*(branch.pp) == NIL);
        *(branch.pp) = cell_ref(doelse); // join up
#endif

        branch.pp = &(doelse->_.cons.car); // iffalse path
        assert(*(branch.pp) == NIL);
        compile1void(iffalse, &branch, cep);
#if 1 // TODO remove alternative
        addfixup(branch.pp, fix);
#else
        // TODO circular ref, should be weak or somesuch
        // TODO jump to doelse should really be one beyond, if possible
        assert(*(branch.pp) == NIL);
        *(branch.pp) = cell_ref(doelse); // join up
#endif
    }
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_and(cell *args, struct fixup *fix, struct compile_env *cep) {
    cell *test;
    cell *pushf = NIL;
    struct fixup push;
    push.pp = &pushf;
    push.chain = NULL;

    while (list_pop(&args, &test)) {
        int bool = -1;
        if (compile2constant(test, &test, cep) && peek_boolean(test, &bool)) {
            // test condition is constant
            cell_unref(test);
        } else {
            compilenc1void(test, fix, cep);
        }
        if (args == NIL) { // last item can be left as is
            if (bool >= 0) add2prog(c_DOQPUSH, cell_boolean(bool), fix);
            break;
        }
        if (bool == 0) {
            cell_unref(args); // false, so forget about rest
            args = NIL ;
            // TODO can use pushf instead?
            add2prog(c_DOQPUSH, cell_ref(hash_f), fix);

        } else if (bool == 1) {
            // continue to next test

        } else {
            // more items, so need a "push false"
            if (pushf == NIL) {
                add2prog(c_DOQPUSH, cell_ref(hash_f), &push); // make NIL dummy for iffalse
            } else {
                cell_ref(pushf); // one more ref
            }
            {
                cell *testp = add2prog(c_DOCOND, NIL, fix); // have NIL dummy for iftrue, filled in below
                fixprog(pushf, fix); // iffalse, go to push false
                fix->pp = &(testp->_.cons.car); // iftrue, continue
            }
        }
    }
    if (pushf != NIL) {
        // patch up missing link
#if 1 // TODO remove alternative
        addfixup(push.pp, fix);
#else
        cell *noop = add2prog(c_DONOOP, NIL, fix); // TODO can be improved
        *(push.pp) = cell_ref(noop); // join up
#endif
    }
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_or(cell *args, struct fixup *fix, struct compile_env *cep) {
    cell *test;
    cell *pusht = NIL;
    struct fixup push;
    push.pp = &pusht;
    push.chain = NULL;

    while (list_pop(&args, &test)) {
        int bool = -1;
        if (compile2constant(test, &test, cep) && peek_boolean(test, &bool)) {
            // test condition is constant
            cell_unref(test);
        } else {
            compilenc1void(test, fix, cep);
        }
        if (args == NIL) { // last item does not need to be read
            if (bool >= 0) add2prog(c_DOQPUSH, cell_boolean(bool), fix);
            break;
        }
        if (bool == 1) {
            cell_unref(args); // true, so forget about rest
            args = NIL ;
            // TODO can use pusht instead?
            add2prog(c_DOQPUSH, cell_ref(hash_t), fix);

        } else if (bool == 0) {
            // continue to next test

        } else {
            // more items, so need a "push true"
            if (pusht == NIL) {
                add2prog(c_DOQPUSH, cell_ref(hash_t), &push); // make NIL dummy for iftrue
            } else {
                cell_ref(pusht); // one more ref
            }
            add2prog(c_DOCOND, pusht, fix); // go push true iftrue
        }
    }
    if (pusht != NIL) {
        // patch up missing link
#if 1 // TODO remove alternative
        addfixup(push.pp, fix);
#else
        cell *noop = add2prog(c_DONOOP, NIL, fix); // TODO can be improved
        *(push.pp)= cell_ref(noop); // join up
#endif
    }
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_defq(cell *args, struct fixup *fix, struct compile_env *cep) {
    cell *nam;
    cell *val;

    if (!arg2(args, &nam, &val)) {
        return 0; // error
    }

    if (cep) {
        if (!assoc_set_local(cep->vars, cell_ref(nam), cell_ref(val))) {
            cell_unref(error_rt1("cannot redefine local value", nam));
            cell_unref(val);
            return 0;
        } else {
            ++(cep->locals);
        }
    } else {
        // TODO do what if define is global?
    }

    compile1void(val, fix, cep);
    add2prog(c_DODEFQ, nam, fix);

    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_refq(cell *args, struct fixup *fix, struct compile_env *cep) {
    cell *nam;
    cell *val;

    if (!arg2(args, &val, &nam)) {
        return 0; // error
    }

    compile1void(val, fix, cep);
    add2prog(c_DOREFQ, nam, fix);
    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_lambda(cell *args, struct fixup *fix, struct compile_env *cep) {
    cell *prog;
    cell *paramlist;
    cell *cp;
    if (!list_pop(&args, &paramlist)) {
        cell_unref(error_rt1("Missing function parameter list", args));
            return 0;
    }

    // prepare a new level of compile environment
    cep = compile_env_new(cep, cell_assoc());

    // TODO consider moving to parser
    // check for proper and for duplicate parameters
    cp = paramlist;
    while (cp) {
        cell *param;

        assert(cell_is_list(cp));
        param = cell_car(cp);
        if (cell_is_label(param)) {
	    // TODO dirty trick for compiling default value
	    cell *val = NIL;
            cell *defval = param->_.cons.cdr;
            param->_.cons.cdr = NIL;

	    if (compile2constant(defval, &val, cep)) {
		param->_.cons.cdr = val;
	    } else {
		param->_.cons.cdr = error_rt1("default value is not a constant", defval);
	    }
            param = param->_.cons.car;
        }
        if (param == hash_ellip) {
            // TODO what to do about this?
            if (cell_cdr(cp)) {
                cell_unref(error_rt1("ellipsis must be last parameter", cp));
                cp = NIL;
            }
            if (!assoc_set(cep->vars, cell_ref(hash_ellip), NIL)) {
                assert(0);
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
    prog = compile_list(args, cep);

    cp = cell_lambda(paramlist, prog);

    if (cep->ref_closure > 0) { // function needs a closure?
        // closure dealt with at runtime
        add2prog(c_DOLAMB, cp, fix);
    } else {
        add2prog(c_DOQPUSH, cp, fix);
    }

    if (opt_showcode) { // debug?
        printf("\nlambda: params=%d, locals=%d, refs: local=%d, closure=%d, global=%d\nprog=",
                 cep->params, cep->locals, cep->ref_local, cep->ref_closure, cep->ref_global);
        debug_writeln(prog);
    }

    compile_env_drop(&cep);

    return 1;
}

// return 1 if something was pushed, 0 otherwise
static int compile1_apply(cell *args, struct fixup *fix, struct compile_env *cep) {
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

    compile1void(tailargs, fix, cep);
    compile1void(fun, fix, cep);
    add2prog(c_DOAPPLY, NIL, fix);
    return 1;
}

// compile, substituting void value in case of error
static void compile1void(cell *item, struct fixup *fix, struct compile_env *cep) {
    if (!compile1(item, fix, cep)) {
        add2prog(c_DOQPUSH, cell_void(), fix); // error
    }
}

// like compile1void(), but do not look for constants initially
static void compilenc1void(cell *item, struct fixup *fix, struct compile_env *cep) {
    if (!compile1nc(item, fix, cep)) {
        add2prog(c_DOQPUSH, cell_void(), fix); // error
    }
}

// attempt to make constant
// return 1 if success, consume item, and set reffed *valp, but no other side effects
// otherwise return 0 and do nothing
static int compile2constant(cell *item, cell **valp, struct compile_env *cep) {

    switch (item ? item->type : c_LIST) {
    case c_FUNC: // function call
        {
            cell *funs = item->_.cons.car;
            cell *args = item->_.cons.cdr;

            if (funs == hash_quote) { // #quote is special case
                if (cell_is_list(args) && cell_cdr(args)==NIL) {
		    *valp = cell_ref(cell_car(args));
                    cell_unref(item);
		    return 1;
		}
            }
            // TODO optimize hash_if, hash_or, hash_and and probably more

            {
                cell *fun = NIL;
                cell *argdef = NIL;
                int n;

                if (!compile2constant(cell_ref(funs), &fun, cep)) {
                    cell_unref(funs);
                    return 0;
                }
                if (!fun->pure) {
                    // cannot fold impure function
                    cell_unref(fun);
                    return 0;
                }
                // TODO for #plus and #times (and others) it is possible to order and make some constants
                // TODO check also each variable for purity

                if ((n = compile2constant_args(cell_ref(args), &argdef, cep)) < 0) {
                    cell_unref(fun);
                    cell_unref(args);
                    return 0;
                }

                // TODO deal with all sorts of functions
                switch (fun->type) {
                case c_CFUN2:
                    if (n == 2) {
                        cell *(*def)(cell *, cell *) = fun->_.cfun2 .def;
                        cell *arg1;
                        cell *arg2;
                        list_pop(&argdef, &arg1);
                        list_pop(&argdef, &arg2);
                        *valp = (*def)(arg1, arg2);
                        cell_unref(fun);
                        cell_unref(item);
                        return 1;
                    }

                case c_CFUN1:
                    if (n == 1) {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell *arg;
                        list_pop(&argdef, &arg);
                        *valp = (*def)(arg);
                        cell_unref(fun);
                        cell_unref(item);
                        return 1;
                    }
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        *valp = (*def)(argdef);
                        cell_unref(fun);
                        cell_unref(item);
                        return 1;
                    }
                    break;

                case c_CFUNR:
                    // these functions are never pure
                    assert(0);
                    cell_unref(fun);
                    cell_unref(item);
                    return 0;

                case c_CLOSURE:
                case c_CLOSURE0:
                case c_CLOSURE0T:
                default:
                    break;
                }
                cell_unref(fun);
                cell_unref(argdef);
                return 0;
            }
        }
	break;

    case c_SYMBOL:
        // look for non-global definition
        if (cep) {
            struct compile_env *e = cep;
            do {
                cell *val;
                if (assoc_get(e->vars, item, &val)) {
                    cell *val2 = NIL;
                    // can optimize away if pure value
                    // TODO any chance of infinite recursion here?
                    if (val != hash_undef && compile2constant(val, &val2, e)) {
                        *valp = val2;
                        cell_unref(item);
                        return 1;
                    }
                    cell_unref(val);
		    return 0;
                }
                // levels above
                e = e->prev;
            } while (e);
        }
	// must be global
        if (item->_.symbol.nam && item->_.symbol.nam[0]=='#') {
            // built in known global
            *valp = cell_ref(item->_.symbol.val);
            cell_unref(item);
            return 1;
        }
        // TODO more optimization
        break;

    case c_LABEL: // car is label, cdr is expr
        {
            cell *label = item->_.cons.car;
            cell *expr = item->_.cons.cdr;
            cell *val = NIL;
            if (!compile2constant(cell_ref(expr), &val, cep)) {
                cell_unref(expr);
                return 0;
            }
            if (cell_is_symbol(label) || cell_is_number(label)) {
                cell_ref(label);
            } else {
                cell *labelval = NIL;
                if (!compile2constant(cell_ref(label), &labelval, cep)) {
                    cell_unref(val);
                    cell_unref(label);
                    return 0;
                }
                label = labelval;
            }
            *valp = cell_label(label, val);
            cell_unref(item);
            return 1;
        }
        break;

    case c_RANGE: // car is lower, cdr is upper bound; both may be NIL
        {
            cell *lower = item->_.cons.car;
            cell *upper = item->_.cons.cdr;
            cell *lowerval = NIL;
            cell *upperval = NIL;
            if (lower && !compile2constant(cell_ref(lower), &lowerval, cep)) {
                cell_unref(lower);
                return 0;
            }
            if (upper && !compile2constant(cell_ref(upper), &upperval, cep)) {
                cell_unref(lowerval);
                cell_unref(upper);
                return 0;
            }
            *valp = cell_range(lowerval, upperval);
        }
        cell_unref(item);
        return 1;

    case c_STRING:
    case c_NUMBER:
        *valp = item;
        return 1;

    default:
	break;
    }
    return 0;
}

// return 1 if something was pushed, 0 otherwise
// item is always consumed
static int compile1(cell *item, struct fixup *fix, struct compile_env *cep) {
    cell *val = NIL;
    if (compile2constant(item, &val, cep)) {
        add2prog(c_DOQPUSH, val, fix);
        return 1;
    }
    return compile1nc(item, fix, cep);
}

// same as compile1(), but no check for constant initially
// this is just to avoid double work
static int compile1nc(cell *item, struct fixup *fix, struct compile_env *cep) {

    switch (item ? item->type : c_LIST) {
    case c_FUNC: // function call
        {
            cell *fun = cell_ref(item->_.cons.car); // TODO make split function for this?
            cell *args = cell_ref(item->_.cons.cdr);
            cell_unref(item);

#if 0 // TODO here only if qoute with not 1 arg
            if (fun == hash_quote) { // #quote is special case
                cell *thing;
                cell_unref(fun);
                if (!arg1(args, &thing)) {
                    return 0;
                }
                // TODO should never end up here...
                add2prog(c_DOQPUSH, thing, fix);
                return 1;
            } else 
#endif
            if (fun == hash_defq) { // #defq is special case
                cell_unref(fun);
                return compile1_defq(args, fix, cep);

            } else if (fun == hash_refq) { // #refq is special case
                cell_unref(fun);
                return compile1_refq(args, fix, cep);

            } else if (fun == hash_if) { // #if is special case
                cell_unref(fun);
                return compile1_if(args, fix, cep);

            } else if (fun == hash_and) { // #and is special case
                cell_unref(fun);
                return compile1_and(args, fix, cep);

            } else if (fun == hash_or) { // #or is special case
                cell_unref(fun);
                return compile1_or(args, fix, cep);

            } else if (fun == hash_lambda) { // #lambda is special case
                cell_unref(fun);
                return compile1_lambda(args, fix, cep);

            } else if (fun == hash_apply) { // #apply is special case
                cell_unref(fun);
                return compile1_apply(args, fix, cep);

            } else {
		cell *def = NIL;
                int n = compile_args(args, fix, cep);
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
                    compile1void(fun, fix, cep);
                    node = add2prog(c_DOCALLN, NIL, fix);
                    node->_.calln.narg = n;
                    assert(fix->pp == &(node->_.calln.cdr)); // TODO assumption
                } else {
                    // see if known internal symbol
                    if (!compile2constant(fun, &def, cep)) {
                        // need to compile function ref, pushing on stack
                        compilenc1void(fun, fix, cep);
                    }
                    add2prog(t, def, fix);
                }

                return 1;
            }
        }

    case c_SYMBOL:
        // look for non-global definition
        if (cep) {
            struct compile_env *e = cep;
            do {
                cell *val;
                if (assoc_get(e->vars, item, &val)) {
                    // TODO: can optimize away if pure value
                    if (e == cep) {
                        ++(cep->ref_local);
                    } else {
                        ++(cep->ref_closure);
                    }
                    cell_unref(val);
                    add2prog(c_DOLPUSH, item, fix);
                    return 1;
                }
                // levels above
                e = e->prev;
            } while (e);

            ++(cep->ref_global);
        }
        add2prog(c_DOGPUSH, item, fix); // must be global
        return 1;

    case c_LABEL: // car is label, cdr is expr
        {
            cell *label;
            cell *val;
            label_split(item, &label, &val);
            compile1void(val, fix, cep);
            if (cell_is_symbol(label) || cell_is_number(label)) {
                add2prog(c_DOLABEL, label, fix);
            } else {
                compile1void(label, fix, cep);
                add2prog(c_DOLABEL, NIL, fix);
            }
        }
        return 1;

    case c_RANGE: // car is lower, cdr is upper bound; both may be NIL
        {
            cell *lower;
            cell *upper;
            range_split(item, &lower, &upper);
            if (upper == NIL) {
                add2prog(c_DOQPUSH, NIL, fix); // TODO can optimize..
            } else {
                compile1void(upper, fix, cep);
            }
            if (lower == NIL) {
                add2prog(c_DOQPUSH, NIL, fix);
            } else {
                compile1void(lower, fix, cep);
            }
            add2prog(c_DORANGE, NIL, fix);
        }
        return 1;

    case c_SPECIAL:
    case c_CLOSURE:
    case c_CLOSURE0:
    case c_CLOSURE0T:
        add2prog(c_DOQPUSH, item, fix);
        return 1;

 // case c_STRING: // compile2constant
 // case c_NUMBER: // compile2constant
    default:
        cell_unref(error_rt1("cannot compile", item));
        return 0;
    }
}

// compile list of expressions, leaving a value on stack
static cell *compile_list(cell *tree, struct compile_env *cep) {
    cell *leavevalue = cell_error(); // assume error
    cell *item;
    int stacklevel = 0;
    cell *result = NIL;
    struct fixup fix;
    fix.pp = &result;
    fix.chain = NULL;

    while (list_pop(&tree, &item)) {
        cell *val = NIL;
        if (stacklevel > 0) {
            add2prog(c_DOPOP, NIL, &fix); // TODO inefficient
            --stacklevel;
        }
        if (!compile2constant(item, &val, cep)) {
            if (compile1nc(item, &fix, cep)) ++stacklevel;
            val = cell_error(); // assume error
        }
        cell_unref(leavevalue);
        leavevalue = val;
    }
    if (stacklevel == 0) {
        add2prog(c_DOQPUSH, leavevalue, &fix);
    } else {
        cell_unref(leavevalue);
    }
    return result; 
}

// if total failure, result is NIL
// TODO deal with that
cell *compile(cell *item, cell *env0) {
    cell *result = NIL;
    struct compile_env *cep = NULL;
    struct fixup fix;
    fix.pp = &result;
    fix.chain = NULL;
    if (env0) {
        // prepare a level of compile environment for 
        // any pre-existing runtime environment
        struct compile_env **pp = &cep;
        do {
            *pp = compile_env_new(NULL, cell_ref(env_assoc(env0)));
            pp = &((*pp)->prev);
            // also include any continuations
        } while ((env0 = env_cont_env(env0)));
    }
    compile1(item, &fix, cep);
    // release any environments
    while (cep) compile_env_drop(&cep);

    return result; 
}

// compile a function call, allowing for N arguments pushed on stack
cell *compile_thunk_n(cell *func, int nargs) {
    cell *prog;
    cell *args = NIL;
    int n;
    for (n = 0; n < nargs; ++n) { // add dummy args
        args = cell_list(cell_integer(-1), args);
    }
    prog = compile(cell_func(func, args), NIL);
    for (n = 0; n < nargs; ++n) { // strip off args
        cell *np;
        assert(prog && prog->type == c_DOQPUSH);
        np = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = np;
    }
    return prog;
}


// push already evaluated arguments last to first, return count
static int push_args(cell *args, struct fixup *fix) {
    int n = 0;
    cell *val;
    if (list_pop(&args, &val)) {
        n = 1 + push_args(args, fix);
        add2prog(c_DOQPUSH, val, fix);
    }
    return n;
}

// make code for a known function call, allowing for supplied list of evaluated arguments
cell *known_thunk_a(cell *func, cell *args) {
    cell *prog = NIL;
    int n;
    struct fixup fix;
    fix.pp = &prog;
    fix.chain = NULL;
    n = push_args(args, &fix);
    switch (n) {
    case 1:
        add2prog(c_DOCALL1, func, &fix);
        break;
    case 2:
        add2prog(c_DOCALL2, func, &fix);
        break;
    default:
        {
            cell *node;
            add2prog(c_DOQPUSH, func, &fix);
            node = add2prog(c_DOCALLN, NIL, &fix);
            node->_.calln.narg = n;
        }
        break;
    }
    return prog;
}
