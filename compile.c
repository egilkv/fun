/*  TAB-P
 *
 *
 */

#include <assert.h>

#include "cell.h"
#include "node.h"
#include "cmod.h"
// #include "cfun.h"
#include "qfun.h"
#include "err.h"

#if HAVE_COMPILER

static int compile1(cell *prog, cell ***nextpp);

static void add2prog(enum cell_t type, cell *car, cell ***nextpp) {
    **nextpp = newnode(type);
    (**nextpp)->_.cons.car = car;
    (**nextpp)->_.cons.cdr = NIL;
    (*nextpp) = &((**nextpp)->_.cons.cdr);
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
static int compile1_cond(cell *args, cell ***nextpp) {
    cell *cond;
    cell *iftrue;
    cell *iffalse;
    if (!list_pop(&args, &cond)) {
        cell_unref(error_rt0("missing condition for if"));
        return 0; // empty #if
    }
    if (!compile1(cond, nextpp)) {
        cell_unref(args);
        return 0; // bad #if condition, assume error message has been issued
    }
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
        cell **noopp;
        add2prog(c_DOCOND, NIL, nextpp); // have NIL dummy for iftrue
        if (!compile1(iffalse, nextpp)) {
            compile1(cell_ref(hash_void), nextpp); // dummy replacement
        }
        condp = &((*condp)->_.cons.car); // iftrue path
        assert(*condp == NIL);
        if (!compile1(iftrue, &condp)) {
            compile1(cell_ref(hash_void), &condp); // dummy replacement
        }
        noopp = *nextpp;
        add2prog(c_DONOOP, NIL, nextpp); // TODO can be improved
        *condp = cell_ref(*noopp); // join up
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
    if (!compile1(val, nextpp)) {
        compile1(cell_ref(hash_void), nextpp); // dummy replacement
    }
    add2prog(c_DODEFQ, nam, nextpp);
    return 1;
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

            } else if (fun == hash_if) { // #if is special case
                cell_unref(fun);
                return compile1_cond(args, nextpp);

            } else {
                int n = compile_args(args, nextpp);
                enum cell_t t;

                switch (n) {
                case 0:
                    t = c_DOCALL0;
                    break;
                case 1:
                    t = c_DOCALL1;
                    break;
                case 2:
                    t = c_DOCALL2;
                    break;
                default: // TODO very wrong...
                case 3:
                    t = c_DOCALL3;
                    break;
                }
                add2prog(t, fun, nextpp);

                return 1;
            }
        }

    case c_SYMBOL: // TODO if known already, we can look it up now
        add2prog(c_DOEPUSH, prog, nextpp);
        return 1;
    case c_STRING:
    case c_NUMBER:
    case c_LIST:    // TODO really?
    case c_VECTOR:  // TODO really?
    case c_ASSOC:   // TODO really?
        add2prog(c_DOQPUSH, prog, nextpp);
        return 1;
    default:
        cell_unref(error_rt1("cannot compile", prog));
        return 0;
    }
}

cell *compile(cell *prog) {
    cell *result = NIL;
    cell **nextp = &result;
    compile1(prog, &nextp);
    return result;
}

#endif // HAVE_COMPILER



