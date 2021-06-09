/*  TAB-P
 *
 *
 */

#include <assert.h>

#include "cell.h"
#include "eval.h"
#include "cmod.h" // get_boolean
#include "err.h"

#if HAVE_COMPILER

// run program, return result
cell *run(cell *prog) {
    cell *next;
    cell *stack = NIL;
    cell *env = NIL;

    while (prog) switch (prog->type) {

    case c_DOQPUSH:       // push car, cdr is next
        stack = cell_list(cell_ref(prog->_.cons.car), stack);
        next = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = next;
        break;

    case c_DOEPUSH:       // eval and push car, cdr is next
        next = cell_ref(prog->_.cons.car);
        next = eval(next, &env);
        stack = cell_list(next, stack);

        next = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = next;
        break;

    case c_DOCALL0:       // car is closure or function, pop 0 args, push result
        {
            cell *fun = cell_ref(prog->_.cons.car);
            cell *result;
            fun = eval(fun, &env); // TODO consider doing evaluation earlier
            switch (fun ? fun->type : c_LIST) {
            case c_CFUN0:
                {
                    cell *(*def)(void) = fun->_.cfun0.def;
                    cell_unref(fun);
                    result = (*def)();
                }
                break;
            case c_CFUNN:
                {
                    cell *(*def)(cell *) = fun->_.cfun1.def;
                    cell_unref(fun);
                    result = (*def)(NIL);
                }
                break;
            default:
                result = error_rt1("not a function with 0 args", fun);
            }
            stack = cell_list(result, stack);
        }
        next = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = next;
        break;

    case c_DOCALL1:       // car is closure or function, pop 1 arg, push result
        {
            cell *fun = cell_ref(prog->_.cons.car);
            cell *result;
            cell *arg;
            fun = eval(fun, &env); // TODO consider doing evaluation earlier
            if (!list_pop(&stack, &arg)) {
                assert(0);
            }
            switch (fun ? fun->type : c_LIST) {
            case c_CFUN1:
                {
                    cell *(*def)(cell *) = fun->_.cfun1.def;
                    cell_unref(fun);
                    result = (*def)(arg);
                }
                break;
            case c_CFUNN:
                {
                    cell *(*def)(cell *) = fun->_.cfun1.def;
                    cell_unref(fun);
                    result = (*def)(cell_list(arg,NIL));
                }
                break;
            default:
                cell_unref(arg);
                result = error_rt1("not a function with 1 arg", fun);
            }
            stack = cell_list(result, stack);
        }
        next = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = next;
        break;

    case c_DOCALL2:       // car is closure or function, pop 2 args, push result
        {
            cell *fun = cell_ref(prog->_.cons.car);
            cell *result;
            cell *arg1, *arg2;
            fun = eval(fun, &env); // TODO consider doing evaluation earlier
            if (!list_pop(&stack, &arg1)) {
                assert(0);
            }
            if (!list_pop(&stack, &arg2)) {
                assert(0);
            }
            switch (fun ? fun->type : c_LIST) {
            case c_CFUN2:
                {
                    cell *(*def)(cell *, cell *) = fun->_.cfun2.def;
                    cell_unref(fun);
                    result = (*def)(arg1, arg2);
                }
                break;
            case c_CFUNN:
                {
                    cell *(*def)(cell *) = fun->_.cfun1.def;
                    cell_unref(fun);
                    result = (*def)(cell_list(arg1,cell_list(arg2,NIL)));
                }
                break;
            default:
                cell_unref(arg1);
                cell_unref(arg2);
                result = error_rt1("not a function with 2 args", fun);
            }
            stack = cell_list(result, stack);
        }
        next = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = next;
        break;

    case c_DOCALL3:       // car is closure or function, pop 3 args, push result
        {
            cell *fun = cell_ref(prog->_.cons.car);
            cell *result;
            cell *arg1, *arg2, *arg3;
            fun = eval(fun, &env); // TODO consider doing evaluation earlier
            if (!list_pop(&stack, &arg1)) {
                assert(0);
            }
            if (!list_pop(&stack, &arg2)) {
                assert(0);
            }
            if (!list_pop(&stack, &arg3)) {
                assert(0);
            }
            switch (fun ? fun->type : c_LIST) {
            case c_CFUN3:
                {
                    cell *(*def)(cell *, cell *, cell *) = fun->_.cfun3.def;
                    cell_unref(fun);
                    result = (*def)(arg1, arg2, arg3);
                }
                break;
            case c_CFUNN:
                {
                    cell *(*def)(cell *) = fun->_.cfun1.def;
                    cell_unref(fun);
                    result = (*def)(cell_list(arg1,cell_list(arg2,cell_list(arg3,NIL))));
                }
                break;
            default:
                cell_unref(arg1);
                cell_unref(arg2);
                cell_unref(arg3);
                result = error_rt1("not a function with 3 args", fun);
            }
            stack = cell_list(result, stack);
        }
        next = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = next;
        break;

    case c_DOCOND:        // pop, car if true, cdr else
        {
            cell *cond;
            int bool;
            if (!list_pop(&stack, &cond)) {
                assert(0);
            }
            if (!get_boolean(cond, &bool, NIL)) {
                bool = 1; // TODO assuming true, is this sensible?
            }
            if (bool) {
                next = cell_ref(prog->_.cons.car);
            } else {
                next = cell_ref(prog->_.cons.cdr);
            }
            cell_unref(prog);
            prog = next;
        }
        break;

    case c_DODEFQ:        // car is name, pop value, push result
        {
            cell *name = cell_ref(prog->_.cons.car);
            cell *arg;
            cell *result;
            if (!list_pop(&stack, &arg)) {
                assert(0);
            }
            result = defq(name, arg, &env);
            stack = cell_list(result, stack);
        }
        next = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = next;
        break;

    default:
        cell_unref(error_rt1("not a program", prog));
        prog = NIL;
        break;

    case c_DONOOP:        // cdr is next
        next = cell_ref(prog->_.cons.cdr);
        cell_unref(prog);
        prog = next;
        break;
    }
    assert(env == NIL);
    if (!list_pop(&stack, &next)) {
        next = NIL;
    }
    if (stack) {
        cell_unref(error_rt1("stack imbalance", stack)); // TODO should this happen
    }
    return next;
}

#endif // HAVE_COMPILER
