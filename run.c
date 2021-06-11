/*  TAB-P
 *
 *
 */

#include <assert.h>

#include "cell.h"
#include "run.h"
#include "cmod.h" // get_boolean
#include "err.h"
#include "debug.h"

struct run_env {
    cell *prog;
    cell *stack;
    cell *env;
} ;

// advance program pointer to anywhere
// TODO inline
static void advance_prog_where(cell *where, struct run_env *rep) {
    // TODO inefficient
    cell *next = cell_ref(where);
    cell_unref(rep->prog);
    rep->prog = next;
}

// advance program pointer to cdr
// TODO inline
static void advance_prog(struct run_env *rep) {
    advance_prog_where(rep->prog->_.cons.cdr, rep);
}

// TODO inline
static void push_value(cell *val, struct run_env *rep) {
    rep->stack = cell_list(val, rep->stack);
}

// TODO inline
static cell *pop_value(struct run_env *rep) {
    cell *val = NIL;
    if (!list_pop(&(rep->stack), &val)) {
        assert(0);
    }
    return val;
}

// evalute a symbol
static cell *run_eval(cell *arg, struct run_env *rep) {
    cell *result = NIL;

    if (arg == NIL) {
        return NIL;
    } else switch (arg->type) {
    case c_SYMBOL:
        {
            cell *e = rep->env;
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

static void run_apply(cell *lambda, cell *args, cell *contenv, struct run_env *rep) {
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
            args = NIL;
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
        cell *next = cell_ref((rep->prog)->_.cons.cdr);
#if 1 // TODO tail call enable
        if (rep->env != NIL && next == NIL) {
            // tail end call, can drop previous environment
            cell *newenv = cell_env(cell_ref(env_prev(rep->env)), NIL, newassoc, contenv);
            cell_unref(rep->env);
            rep->env = newenv;
        } else
#endif
        {
            // push one level of environment
            cell *newenv = cell_env(rep->env, next, newassoc, contenv);
            rep->env = newenv;
        }
    }
    // switch to new program
    cell_unref(rep->prog);
    rep->prog = body;
}

// run program, return result
cell *run(cell *prog0) {
    struct run_env re;
    re.prog = prog0;
    re.stack = NIL;
    re.env = NIL;

    for (;;) {
        while (re.prog == NIL) {
            // drop one level of environment
            if (re.env == NIL) {
                cell *result;
                if (!list_pop(&re.stack, &result)) {
                    result = cell_ref(hash_void);
                }
                if (re.stack) {
                    cell_unref(error_rt1("stack imbalance", re.stack)); // TODO should this happen
                }
                return result;
            } else {
                // pop one level of program/environment stack
                cell *contprog = cell_ref(env_prog(re.env));
                cell *prevenv = cell_ref(env_prev(re.env));
                cell_unref(re.env);
                re.env = prevenv;
                re.prog = contprog;
            }
        }

        switch (re.prog->type) {

        case c_DOQPUSH:   // push car, cdr is next
            push_value(cell_ref(re.prog->_.cons.car), &re);
            advance_prog(&re);
            break;

        case c_DOEPUSH:   // eval and push car, cdr is next
            {
                cell *val;
                val = cell_ref(re.prog->_.cons.car);
                val = run_eval(val, &re);
                push_value(val, &re);
                advance_prog(&re);
            }
            break;

        case c_DOCALL1:   // car is closure or function, pop 1 arg, push result
            {
                // function evaluated on stack?
                cell *fun = re.prog->_.cons.car ? cell_ref(re.prog->_.cons.car) : pop_value(&re);
                cell *arg = pop_value(&re);
                cell *result;
                switch (fun ? fun->type : c_LIST) {
                case c_CFUN1:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(arg);
                        push_value(result, &re);
                    }
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(cell_list(arg, NIL));
                        push_value(result, &re);
                    }
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        run_apply(lambda, cell_list(arg, NIL), contenv, &re);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    run_apply(fun, cell_list(arg, NIL), NIL, &re);
                    break;

                default:
                    cell_unref(arg);
                    push_value(error_rt1("not a function with 1 arg", fun), &re);
                    advance_prog(&re);
                    break;
                }
            }
            break;

        case c_DOCALL2:   // car is closure or function, pop 2 args, push result
            {
                // function evaluated on stack?
                cell *fun = re.prog->_.cons.car ? cell_ref(re.prog->_.cons.car) : pop_value(&re);
                cell *arg1 = pop_value(&re);
                cell *arg2 = pop_value(&re);
                cell *result;
                switch (fun ? fun->type : c_LIST) {
                case c_CFUN2:
                    {
                        cell *(*def)(cell *, cell *) = fun->_.cfun2.def;
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(arg1, arg2);
                        push_value(result, &re);
                    }
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(cell_list(arg1,cell_list(arg2,NIL)));
                        push_value(result, &re);
                    }
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        run_apply(lambda, cell_list(arg1,cell_list(arg2, NIL)), contenv, &re);
                    }
                    break;

                case c_CLOSURE0:
                case c_CLOSURE0T:
                    run_apply(fun, cell_list(arg1,cell_list(arg2, NIL)), NIL, &re);
                    break;

                default:
                    cell_unref(arg1);
                    cell_unref(arg2);
                    push_value(error_rt1("not a function with 2 args", fun), &re);
                    advance_prog(&re);
                    break;
                }
            }
            break;

        case c_DOCALLN:   // car is Nargs, pop function, pop N args, push result
            {
                integer_t narg = re.prog->_.calln.narg;
                cell *args = NIL;
                cell **pp = &args;
                cell *fun = pop_value(&re);
                cell *result;
                // function evaluated on stack
                while (narg-- > 0) {
                    cell *a = pop_value(&re);
                    *pp = cell_list(a, NIL);
                    pp = &((*pp)->_.cons.cdr);
                }
                switch (fun ? fun->type : c_LIST) {
                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        // TODO there is also another OP using calln
                        advance_prog_where(re.prog->_.calln.cdr, &re);
                        result = (*def)(args);
                        push_value(result, &re);
                    }
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        run_apply(lambda, args, contenv, &re);
                    }
                    break;

                case c_CLOSURE0:
                case c_CLOSURE0T:
                    run_apply(fun, args, NIL, &re);
                    break;

                default:
                    cell_unref(args);
                    push_value(error_rt1("not a function with N args", fun), &re);
                    advance_prog_where(re.prog->_.calln.cdr, &re);
                    break;
                }
            }
            break;

        case c_DOCOND:    // pop, car if true, cdr else
            {
                int bool;
                cell *cond = pop_value(&re);
                if (!get_boolean(cond, &bool, NIL)) {
                    bool = 1; // TODO assuming true, is this sensible?
                }
                if (bool) {
                    advance_prog_where(re.prog->_.cons.car, &re);
                } else {
                    advance_prog(&re);
                }
            }
            break;

        case c_DODEFQ:    // car is name, pop value, push result
            {
                cell *name = cell_ref(re.prog->_.cons.car);
                cell *arg = pop_value(&re);
                cell *result = defq(name, arg, &re.env);
                push_value(result, &re);
                advance_prog(&re);
            }
            break;

        case c_DOREFQ:    // car is name, pop assoc, push value, cdr is next
            {
                cell *name = cell_ref(re.prog->_.cons.car);
                cell *arg = pop_value(&re);
                cell *result = cfun2_ref(arg, name);
                push_value(result, &re);
                advance_prog(&re);
            }
            break;

        case c_DOLAMB:    // car is cell_lambda, cdr is next
            {
                cell *cp = cell_ref(re.prog->_.cons.car);
                if (re.env != NIL) {
                    cp = cell_closure(cp, cell_ref(re.env));
                }
                // cell_unref(error_rt1("and now what?", cp));
                push_value(cp, &re);
                advance_prog(&re);
            }
            break;

        case c_DOLABEL:   // pop expr, car is label or NIL for pop, push value, cdr is next
            {
                cell *label = re.prog->_.cons.car ? cell_ref(re.prog->_.cons.car) : pop_value(&re);
                cell *val = pop_value(&re);
                cell *result = cell_label(label, val);
                push_value(result, &re);
                advance_prog(&re);
            }
            break;

        case c_DORANGE:   // pop lower, pop upper, push result, cdr is next
            {
                cell *lower = pop_value(&re);
                cell *upper = pop_value(&re);
                cell *result = cell_range(lower, upper);
                push_value(result, &re);
                advance_prog(&re);
            }
            break;

        case c_DOAPPLY:   // pop tailarg, pop func, push result, cdr is next
            {
                cell *fun = pop_value(&re);
                cell *tailarg = pop_value(&re);
                cell *result;
                if (tailarg != NIL && !cell_is_list(tailarg)) {
                    cell_unref(fun);
                    push_value(error_rt1("apply argument not a list", tailarg), &re);
                    advance_prog(&re);

                } else switch (fun ? fun->type : c_LIST) {
                case c_CFUN1:
                    {
                        cell *arg;
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        if (!list_pop(&tailarg, &arg)) {
                            arg = cell_ref(cell_void()); // error
                        }
                        arg0(tailarg);
                        advance_prog(&re);
                        result = (*def)(arg);
                        push_value(result, &re);
                    }
                    break;

                case c_CFUN2:
                    {
                        cell *arg1;
                        cell *arg2;
                        cell *(*def)(cell *, cell *) = fun->_.cfun2.def;
                        cell_unref(fun);
                        if (!list_pop(&tailarg, &arg1)) {
                            arg1 = cell_ref(cell_void()); // error
                            arg2 = cell_ref(cell_void()); // error
                        } else if (!list_pop(&tailarg, &arg2)) {
                            arg2 = cell_ref(cell_void()); // error
                        }
                        arg0(tailarg);
                        advance_prog(&re);
                        result = (*def)(arg1, arg2);
                        push_value(result, &re);
                    }
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(tailarg);
                        push_value(result, &re);
                    }
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        run_apply(lambda, tailarg, contenv, &re);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    run_apply(fun, tailarg, NIL, &re);
                    break;

                default:
                    cell_unref(tailarg);
                    push_value(error_rt1("not a function", fun), &re);
                    advance_prog(&re);
                    break;
                }
            }
            break;

        case c_DOPOP:     // cdr is next
            cell_unref(pop_value(&re));
            advance_prog(&re);
            break;

        case c_DONOOP:    // cdr is next
            advance_prog(&re);
            break;

        default:
            cell_unref(error_rt1("not a program", re.prog));
            re.prog = NIL;
            break;

        }
    }
}

