/*  TAB-P
 *
 *
 */

#include <assert.h>

#include "cell.h"
#include "run.h"
#include "node.h" // newnode
#include "cmod.h" // get_boolean
#include "err.h"
#include "debug.h"

struct run_env {
    cell *prog;
    cell *stack;
    cell *env;
    struct run_env *save; // TODO probably remoce
} ;

static void run_pushprog(cell *body, cell *newassoc, cell *contenv, struct run_env *rep);

// advance program pointer to anywhere
static INLINE void advance_prog_where(cell *where, struct run_env *rep) {
    // TODO inefficient
    cell *next = cell_ref(where);
    cell_unref(rep->prog);
    rep->prog = next;
}

// advance program pointer to cdr
static INLINE void advance_prog(struct run_env *rep) {
    advance_prog_where(rep->prog->_.cons.cdr, rep);
}

static INLINE void push_value_stackp(cell *val, cell **stackp) {
    *stackp = cell_list(val, *stackp);
}

static INLINE void push_value(cell *val, struct run_env *rep) {
    push_value_stackp(val, &(rep->stack));
}

static INLINE cell *pop_value(struct run_env *rep) {
    cell *val = NIL;
    if (!list_pop(&(rep->stack), &val)) {
        assert(0);
    }
    return val;
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

    run_pushprog(body, newassoc, contenv, rep);
}

static void run_pushprog(cell *body, cell *newassoc, cell *contenv, struct run_env *rep) {
    cell *next = cell_ref(rep->prog);

    // save current program pointer in environment
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
    // switch to new program
    cell_unref(rep->prog);
    rep->prog = body;
}

// the one run environment
struct run_env *run_environment = 0;

// run a program from anywhere
void run_async(cell *prog) {
    // TODO implement async
    cell_unref(run_main(prog));
}

#if 0
// insert a program
// TODO not used, may ont work
void run_also(cell *prog) {
    if (run_environment) {
        run_pushprog(prog, NIL, NIL, run_environment);
    } else {
        cell_unref(run_main(prog));
    }
}
#endif

// apply a function from extern
void run_main_apply(cell *lambda, cell *args) {
#if 0
    if (run_environment) {
        run_apply(lambda, args, NIL, run_environment);
    } else 
#endif
    {
        // TODO have to make silly program
        cell *prog = NIL;
        cell **pp = &prog;

        *pp = newnode(c_DOQPUSH);
        (*pp)->_.cons.car = args;
        pp = &(*pp)->_.cons.cdr;

        *pp = newnode(c_DOQPUSH);
        (*pp)->_.cons.car = lambda;
        pp = &(*pp)->_.cons.cdr;

        *pp = newnode(c_DOAPPLY);

        cell_unref(run_main(prog));
    }
}

// main run program facility, return result
cell *run_main(cell *prog) {
    struct run_env re;

    re.prog = prog;
    re.stack = NIL;
    re.env = NIL;
    re.save = run_environment; // mostly NULL
    run_environment = &re;

    for (;;) {
        while (re.prog == NIL) {
            // drop one level of environment
            if (re.env == NIL) {
                cell *result;
                if (!list_pop(&re.stack, &result)) {
                    result = cell_void();
                }
                if (re.stack) {
                    cell_unref(error_rt1("stack imbalance", re.stack)); // TODO should this happen?
                }
                run_environment = re.save;
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

	case c_DOGPUSH:   // car is global symbol, push value, cdr is next
            {
		cell *sym = re.prog->_.cons.car;
		assert(cell_is_symbol(sym));
		push_value(cell_ref(sym->_.symbol.val), &re);
                advance_prog(&re);
            }
	    break;

        case c_DOLPUSH:   // car is local, push value, cdr is next
            {
		cell *sym = re.prog->_.cons.car;
                cell *e = re.env;
                cell *val;
		assert(cell_is_symbol(sym));
                while (e) {
                    if (assoc_get(env_assoc(e), sym, &val)) {
                        break;
                    }
                    e = env_cont_env(e);
                }
                if (!e) {
                    val = error_rt1("no local", cell_ref(sym));
                }
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
                        cell **stackp = &(re.stack);
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(arg);
                        push_value_stackp(result, stackp); // use stackp in case re changed
                    }
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell **stackp = &(re.stack);
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(cell_list(arg, NIL));
                        push_value_stackp(result, stackp);
                    }
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        advance_prog(&re);
                        run_apply(lambda, cell_list(arg, NIL), contenv, &re);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    advance_prog(&re);
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
                        cell **stackp = &(re.stack);
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(arg1, arg2);
                        push_value_stackp(result, stackp);
                    }
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell **stackp = &(re.stack);
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(cell_list(arg1,cell_list(arg2,NIL)));
                        push_value_stackp(result, stackp);
                    }
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        advance_prog(&re);
                        run_apply(lambda, cell_list(arg1,cell_list(arg2, NIL)), contenv, &re);
                    }
                    break;

                case c_CLOSURE0:
                case c_CLOSURE0T:
                    advance_prog(&re);
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
                        cell **stackp = &(re.stack);
                        cell_unref(fun);
                        // TODO there is also another OP using calln
                        advance_prog_where(re.prog->_.calln.cdr, &re);
                        result = (*def)(args);
                        push_value_stackp(result, stackp);
                    }
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        advance_prog(&re);
                        run_apply(lambda, args, contenv, &re);
                    }
                    break;

                case c_CLOSURE0:
                case c_CLOSURE0T:
                    advance_prog(&re);
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
                cp = cell_closure(cp, cell_ref(re.env));
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
                        cell **stackp = &(re.stack);
                        cell_unref(fun);
                        if (!list_pop(&tailarg, &arg)) {
                            arg = cell_ref(cell_void()); // error
                        }
                        arg0(tailarg);
                        advance_prog(&re);
                        result = (*def)(arg);
                        push_value_stackp(result, stackp);
                    }
                    break;

                case c_CFUN2:
                    {
                        cell *arg1;
                        cell *arg2;
                        cell *(*def)(cell *, cell *) = fun->_.cfun2.def;
                        cell **stackp = &(re.stack);
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
                        push_value_stackp(result, stackp);
                    }
                    break;

                case c_CFUNN:
                    {
                        cell *(*def)(cell *) = fun->_.cfun1.def;
                        cell **stackp = &(re.stack);
                        cell_unref(fun);
                        advance_prog(&re);
                        result = (*def)(tailarg);
                        push_value_stackp(result, stackp);
                    }
                    break;

                case c_CLOSURE:
                    {
                        cell *lambda = cell_ref(fun->_.cons.car);
                        cell *contenv = cell_ref(fun->_.cons.cdr);
                        cell_unref(fun);
                        advance_prog(&re);
                        run_apply(lambda, tailarg, contenv, &re);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    advance_prog(&re);
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
    assert(0);
}

