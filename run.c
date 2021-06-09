/*  TAB-P
 *
 *
 */

#include <assert.h>

#include "cell.h"
#include "eval.h" // defq
#include "cmod.h" // get_boolean
#include "err.h"
#include "debug.h"

#if HAVE_COMPILER

// evalute a symbol
cell *eval_item(cell *arg, cell **envp) {
    cell *result = NIL;

    if (arg == NIL) {
        return NIL;
    } else switch (arg->type) {
    case c_SYMBOL:
        {
            cell *e = *envp;
            for (;;) {
                if (!e) { // global?
                    result = cell_ref(arg->_.symbol.val);
                    break;
                }
                if (assoc_get(env_assoc(e), arg, &result)) {
                    break;
                }
                e = env_cont_env(e);
            }
        }
        cell_unref(arg);
        return result;

    case c_STRING:
    case c_NUMBER:
        // TODO what evaluates to itself?
        return arg;

    default:
        return error_rt1("cannot evaluate", arg);
    }
}

static void apply_run(cell *lambda, cell *args, cell *contenv, cell **progp, cell **envp) {
    int gotlabel = 0;
    cell *nam;
    cell *val;
    cell *params = cell_ref(lambda->_.cons.car);
    cell *body = cell_ref(lambda->_.cons.cdr);
    cell *newassoc = cell_assoc();

    if (lambda->type == c_CLOSURE0T) {
        debug_prints("\n*** "); // trace
    }

    // pick up actual arguments one by one
    while (list_pop(&args, &val)) {

        if (cell_is_list(params) && cell_car(params) == hash_ellip) {
            // special case for ellipsis, place rest on list
            val = cell_list(val, args);
            cell_unref(params);
	    params = NIL;
            nam = cell_ref(hash_ellip);

	} else if (cell_is_label(val)) {
            ++gotlabel;
            label_split(val, &nam, &val);
            if (!cell_is_symbol(nam)) {
                cell_unref(error_rt1("parameter label must be a symbol", nam));
                cell_unref(val);
                continue;
            }
	    // TODO disallow ellipsis here
            // check if label actually exists on parameter list
            // TODO takes much time
            if (!exists_on_list(params, nam)) {
                cell_unref(error_rt1("no such parameter label", nam));
                cell_unref(val);
                continue;
            }

        } else if (gotlabel) {
            if (exists_on_list(params, hash_ellip)) {
		// variadic function mixed with labels
                val = cell_list(val, args);
                cell_unref(params);
                params = NIL;
                nam = cell_ref(hash_ellip);
            } else {
                // TODO this can be implemented, but should it?
                cell_unref(error_rt1("ignore unlabelled argument following labelled", val));
                continue;
            }
        } else {
            // match unlabeled argument to parameter list
            if (!list_pop(&params, &nam)) {
                arg0(cell_list(val, args)); // too many args?
                break;
            }
            if (cell_is_label(nam)) { // a default value is supplied?
                label_split(nam, &nam, NILP);
            }
	    assert(cell_is_symbol(nam));
        }

        if (lambda->type == c_CLOSURE0T) {
            debug_write(nam);
            debug_prints(": ");
            debug_write(val);
        }
	if (!assoc_set(newassoc, nam, val)) {
            // TODO checked earlier, so should not happen
	    cell_unref(val);
	    cell_unref(error_rt1("duplicate parameter name, value ignored", nam));
	}
    }

    // check for any args with no value supplied for?
    while (list_pop(&params, &nam)) {
        // TODO if more than one, have one message
        if (cell_is_label(nam)) { // a default value is supplied?
            label_split(nam, &nam, &val);
            if (assoc_get(newassoc, nam, NILP)) {
                cell_unref(nam);
                cell_unref(val);
            } else {
                // use default value
                // TODO eval when?
                if (!assoc_set(newassoc, nam, val)) {
                    assert(0);
                }
            }
        } else { // no default
            if (assoc_get(newassoc, nam, NILP)) {
                cell_unref(nam);
            } else if (nam == hash_ellip) {
                if (!assoc_set(newassoc, nam, NIL)) { // empty list
                    assert(0);
                }
            } else {
                cell_unref(error_rt1("missing value for", cell_ref(nam)));
                if (!assoc_set(newassoc, nam, cell_void())) { // error
                    assert(0);
                }
            }
        }
    }

    if (lambda->type == c_CLOSURE0T) {
        // debug_prints("\n");
    }
    cell_unref(lambda);

    // save current program pointer in environment
    {
        cell *next = cell_ref((*progp)->_.cons.cdr);
#if 0
        if (*envp != NIL && next == NIL) {
            // tail end call, can drop previous environment
            cell *newenv = cell_env(cell_ref(env_prev(*envp)), NIL, newassoc, contenv);
            cell_unref(*envp);
            *envp = newenv;
        } else
#endif
        {
            // push one level of environment
            cell *newenv = cell_env(*envp, next, newassoc, contenv);
            *envp = newenv;
        }
    }
    // switch to new program
    cell_unref(*progp);
    *progp = body;
}

// run program, return result
cell *run(cell *prog) {
    cell *next;
    cell *stack = NIL;
    cell *env = NIL;

    for (;;) {
        while (prog == NIL) {
            // drop one level of environment
            if (env == NIL) {
                cell *result;
                if (!list_pop(&stack, &result)) {
                    result = cell_ref(hash_void);
                }
                if (stack) {
                    cell_unref(error_rt1("stack imbalance", stack)); // TODO should this happen
                }
                return result;
            } else {
                // pop one level of program/environment stack
                cell *contprog = cell_ref(env_prog(env));
                cell *prevenv = cell_ref(env_prev(env));
                cell_unref(env);
                env = prevenv;
                prog = contprog;
            }
        }

        switch (prog->type) {

        case c_DOQPUSH:   // push car, cdr is next
            stack = cell_list(cell_ref(prog->_.cons.car), stack);
            next = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);
            prog = next;
            break;

        case c_DOEPUSH:   // eval and push car, cdr is next
            next = cell_ref(prog->_.cons.car);
            next = eval_item(next, &env);
            stack = cell_list(next, stack);

            next = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);
            prog = next;
            break;

        case c_DOCALL0:   // car is closure or function, pop 0 args, push result
            {
                cell *fun = prog->_.cons.car;
                cell *result;
                if (fun == NIL) { // function evaluated on stack?
                    if (!list_pop(&stack, &fun)) {
                        assert(0);
                    }
                } else {
                    fun = cell_ref(fun); // known function
                }
                switch (fun ? fun->type : c_LIST) {
                case c_CFUN0:
                    {
                        cell *(*def)(void) = fun->_.cfun0.def;
                        cell_unref(fun);
                        result = (*def)();
                    }
                builtin0:
                    stack = cell_list(result, stack);
                    next = cell_ref(prog->_.cons.cdr);
                    cell_unref(prog);
                    prog = next;
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        result = (*def)(NIL);
                    }
                    goto builtin0;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        apply_run(lambda, NIL, contenv, &prog, &env);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    apply_run(fun, NIL, NIL, &prog, &env);
                    break;

                default:
                    result = error_rt1("not a function with 0 args", fun);
                    goto builtin0;
                }
            }
            break;

        case c_DOCALL1:   // car is closure or function, pop 1 arg, push result
            {
                cell *fun = prog->_.cons.car;
                cell *result;
                cell *arg;
                if (fun == NIL) { // function evaluated on stack?
                    if (!list_pop(&stack, &fun)) {
                        assert(0);
                    }
                } else {
                    fun = cell_ref(fun); // known function
                }
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
                builtin1:
                    stack = cell_list(result, stack);
                    next = cell_ref(prog->_.cons.cdr);
                    cell_unref(prog);
                    prog = next;
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        result = (*def)(cell_list(arg,NIL));
                    }
                    goto builtin1;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        apply_run(lambda, cell_list(arg,NIL), contenv, &prog, &env);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    apply_run(fun, cell_list(arg,NIL), NIL, &prog, &env);
                    break;

                default:
                    cell_unref(arg);
                    result = error_rt1("not a function with 1 arg", fun);
                    goto builtin1;
                }
            }
            break;

        case c_DOCALL2:   // car is closure or function, pop 2 args, push result
            {
                cell *fun = prog->_.cons.car;
                cell *result;
                cell *arg1, *arg2;
                if (fun == NIL) { // function evaluated on stack?
                    if (!list_pop(&stack, &fun)) {
                        assert(0);
                    }
                } else {
                    fun = cell_ref(fun); // known function
                }
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
                builtin2:
                    stack = cell_list(result, stack);
                    next = cell_ref(prog->_.cons.cdr);
                    cell_unref(prog);
                    prog = next;
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        result = (*def)(cell_list(arg1,cell_list(arg2,NIL)));
                    }
                    goto builtin2;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        apply_run(lambda, cell_list(arg1,cell_list(arg2,NIL)), contenv, &prog, &env);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    apply_run(fun, cell_list(arg1,cell_list(arg2,NIL)), NIL, &prog, &env);
                    break;

                default:
                    cell_unref(arg1);
                    cell_unref(arg2);
                    result = error_rt1("not a function with 2 args", fun);
                    goto builtin2;
                }
            }
            break;

        case c_DOCALLN:   // car is Nargs, pop function, pop N args, push result
            {
                integer_t narg = prog->_.calln.narg;
                cell *result;
                cell *fun = NIL;
                cell *args = NIL;
                cell **pp = &args;
                // function evaluated on stack
                if (!list_pop(&stack, &fun)) {
                    assert(0);
                }
                while (narg-- > 0) {
                    cell *a;
                    if (!list_pop(&stack, &a)) {
                        assert(0);
                    }
                    *pp = cell_list(a, NIL);
                    pp = &((*pp)->_.cons.cdr);
                }
                switch (fun ? fun->type : c_LIST) {
                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        result = (*def)(args);
                    }
                builtinN:
                    stack = cell_list(result, stack);
                    next = cell_ref(prog->_.calln.cdr);
                    cell_unref(prog);
                    prog = next;
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        apply_run(lambda, args, contenv, &prog, &env);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    apply_run(fun, args, NIL, &prog, &env);
                    break;

                default:
                    cell_unref(args);
                    result = error_rt1("not a function with N args", fun);
                    goto builtinN;
                }
            }
            break;

        case c_DOCOND:    // pop, car if true, cdr else
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

        case c_DODEFQ:    // car is name, pop value, push result
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

        case c_DOREFQ:    // car is name, pop assoc, push value, cdr is next
            {
                cell *name = cell_ref(prog->_.cons.car);
                cell *arg;
                cell *result;
                if (!list_pop(&stack, &arg)) {
                    assert(0);
                }
                result = cfun2_ref(arg, name);
                stack = cell_list(result, stack);
            }
            next = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);
            prog = next;
            break;

        case c_DOLAMB:    // car is cell_lambda, cdr is next
            {
                cell *cp = cell_ref(prog->_.cons.car);

                // make closure as needed
                if (env != NIL) {
                    cp = cell_closure(cp, cell_ref(env));
                }
                // cell_unref(error_rt1("and now what?", cp));
                stack = cell_list(cp, stack);
            }
            next = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);
            prog = next;
            break;

        case c_DOLABL:    // car is label, pop value, push result, cdr is next
            {
                cell *label = cell_ref(prog->_.cons.car);
                cell *val;
                cell *result;
                if (!list_pop(&stack, &val)) {
                    assert(0);
                }
                result = cell_label(label, val);
                stack = cell_list(result, stack);
            }
            next = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);
            prog = next;
            break;

        case c_DOPOP:     // cdr is next
            {
                cell *pop;
                if (!list_pop(&stack, &pop)) {
                    assert(0);
                }
                cell_unref(pop);
            }
            next = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);
            prog = next;
            break;

        default:
            cell_unref(error_rt1("not a program", prog));
            prog = NIL;
            break;

        case c_DONOOP:    // cdr is next
            next = cell_ref(prog->_.cons.cdr);
            cell_unref(prog);
            prog = next;
            break;
        }
    }
}

#endif // HAVE_COMPILER
