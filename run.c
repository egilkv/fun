/*  TAB-P
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cell.h"
#include "run.h"
#include "node.h" // newnode
#include "cmod.h" // get_boolean
#include "err.h"
#include "debug.h"

// statically allocated run environment
struct current_run_env {
    cell *prog;             // program being run TODO also in env, but here for efficiency
    cell *env;              // current environment
    cell *stack;            // current runtime stack
    int proc_id;            // 0=anonymous, 1=main thread, 2=debugger
    struct current_run_env *save; // previous runtime environments, used when debugging primarily
} ;

// the one run environment
static struct current_run_env *run_environment = 0;

// list of other ready processes
struct proc_run_env *ready_list = 0;

// prepare a new level of compile environment
struct proc_run_env *run_environment_new(cell *prog, cell *env, cell *stack) {
    struct proc_run_env *newenv = malloc(sizeof(struct proc_run_env));
    assert(newenv);
    newenv->prog = prog;
    newenv->env = env;
    newenv->stack = stack;
    newenv->next = NULL;
    newenv->proc_id = 0;
    return newenv;
}

// free up all run environment resources
void run_environment_drop(struct proc_run_env *rep) {
    while (rep) {
        struct proc_run_env *next;
        cell_unref(rep->prog);
        cell_unref(rep->env);
        cell_unref(rep->stack);
        next = rep->next;
        free(rep);
        rep = next;
    }
}

// sweep run environment resources
void run_environment_sweep(struct proc_run_env *rep) {
    while (rep) {
        cell_sweep(rep->prog);
        cell_sweep(rep->env);
        cell_sweep(rep->stack);
        rep = rep->next;
    }
}

// append process to process list
void append_proc_list(struct proc_run_env **pp, struct proc_run_env *rep) {
    while (*pp) {
        pp = &((*pp)->next);
    }
    *pp = rep;
}

// prepend process on process list
void prepend_proc_list(struct proc_run_env **pp, struct proc_run_env *rep) {
    struct proc_run_env *next = *pp;
    *pp = rep;
    rep->next = next;
}

// for debugging
cell *current_run_env() {
    if (!run_environment) {
        cell_unref(error_rt0("breakpoint without run")); // TODO should not happen
        return NIL;
    } else {
        // prevenv and prog are both NIL
        return run_environment->env;
    }
}

static void run_pushprog(cell *body, cell *newassoc, cell *contenv, struct current_run_env *rep);

// advance program pointer to anywhere
static INLINE void advance_prog_where(cell *where, struct current_run_env *rep) {
    // TODO inefficient
    cell *next = cell_ref(where);
    cell_unref(rep->prog);
    rep->prog = next;
}

// advance program pointer to cdr
static INLINE void advance_prog(struct current_run_env *rep) {
    advance_prog_where(rep->prog->_.cons.cdr, rep);
}

static INLINE void push_value_stackp(cell *val, cell **stackp) {
    *stackp = cell_list(val, *stackp);
}

static INLINE void push_value(cell *val, struct current_run_env *rep) {
    if (!rep) {
        assert(0);
    }
    push_value_stackp(val, &(rep->stack));
}

static INLINE cell *pop_value(struct current_run_env *rep) {
    cell *val = NIL;
    if (!list_pop(&(rep->stack), &val)) {
        assert(0);
    }
    return val;
}

// for special cases
void push_stack_current_run_env(cell *val) {
    push_value(val, run_environment);
}

static void run_apply(cell *lambda, cell *args, cell *contenv, struct current_run_env *rep) {
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

static void run_pushprog(cell *body, cell *newassoc, cell *contenv, struct current_run_env *rep) {
    cell *next = cell_ref(rep->prog);

    // save current program pointer in environment
#if 1 // TODO tail call enable

    if (rep->env != NIL && next == NIL) {
        // tail end call, can drop previous environment:
        //   env_prev(rep->env)
        //   env_cont_env(rep->env)
        //   env_prog(rep->env)
        //   env_assoc(rep->env)

        cell *newenv = cell_env(cell_ref(env_prev(rep->env)), cell_ref(env_prog(rep->env)), newassoc, contenv);

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

// apply a function from extern
// TODO review
void run_main_apply(cell *lambda, cell *args) {
    {
        // TODO have to make silly program
        // TODO use stack instead...
        cell *prog = NIL;
        cell **pp = &prog;

        *pp = newnode(c_DOQPUSH);
        (*pp)->_.cons.car = args;
        pp = &(*pp)->_.cons.cdr;

        *pp = newnode(c_DOQPUSH);
        (*pp)->_.cons.car = lambda;
        pp = &(*pp)->_.cons.cdr;

        *pp = newnode(c_DOAPPLY);

        cell_unref(run_main(prog, NIL, NIL, 0));
    }
}

// set run_environment, return next
static struct proc_run_env *set_run_environment(struct proc_run_env *newp) {
    if (newp) {
        struct proc_run_env *next = newp->next;
        run_environment->prog = newp->prog;
        run_environment->env = newp->env;
        run_environment->stack = newp->stack;
        run_environment->proc_id = newp->proc_id;
        free(newp);
        return next;
    } else {
        run_environment->prog = NIL;
        run_environment->env = NIL;
        run_environment->stack = NIL;
        run_environment->proc_id = 0;
        return NULL;
    }
}

// suspend running process, return its descriptor
struct proc_run_env *suspend() {
    struct proc_run_env *susp;
    if (run_environment == NULL) {
        assert(0);
    }
    susp = run_environment_new(run_environment->prog, run_environment->env, run_environment->stack);
    susp->proc_id = run_environment->proc_id;

    if (ready_list != NULL) {
        ready_list = set_run_environment(ready_list);
    } else {
        // nothing more to do
        set_run_environment(NULL);
    }
    return susp;
}

// interrupt running process, inserting new process
void start_process(cell *prog, cell *env, cell *stack) {
    struct proc_run_env *susp;
    if (run_environment == NULL) {
        assert(0);
    }
    susp = run_environment_new(run_environment->prog, run_environment->env, run_environment->stack);
    susp->proc_id = run_environment->proc_id;

    run_environment->prog = prog;
    run_environment->env = env;
    run_environment->stack = stack;
    run_environment->proc_id = 0;

    prepend_proc_list(&ready_list, susp);
}

// dispatch, rotate ready list
int dispatch() {
    if (ready_list == NULL) return 0;

    if (run_environment->prog != NIL 
     || run_environment->env != NIL 
     || (run_environment->stack != NIL && run_environment->proc_id != 0)) {
        // there is something of interest, so suspend it
        struct proc_run_env *prev = suspend();
        append_proc_list(&ready_list, prev);
    } else {
        // pop from top of ready list
        ready_list = set_run_environment(ready_list);
    }
    return 1;
}

// main run program facility, return result
cell *run_main(cell *prog, cell *env0, cell *stack, int proc_id) {
    struct current_run_env re;

    re.prog = prog;
    re.env = env0;
    re.stack = stack;
    re.proc_id = proc_id;
    re.save = run_environment; // should usually be NULL

    run_environment = &re;

    for (;;) {
        while (re.prog == NIL) {
            if (re.env != NIL) {
                // pop one level of program/environment stack
                cell *contprog = cell_ref(env_prog(re.env));
                cell *prevenv = cell_ref(env_prev(re.env));
                cell_unref(re.env);
                re.env = prevenv;
                re.prog = contprog;
            } else if (re.proc_id != proc_id) {
                // a thread stopped
                fprintf(stdout, "*thread %d stopped*\n", re.proc_id); // TODO only in debug mode of some kind
                cell_unref(re.env); // discard environment
                re.env = NIL;
                cell_unref(re.stack); // and results
                re.stack = NIL;
                if (!dispatch()) { // TODO logic is shaky
                    return error_rt0("main thread not ready");
                }
            } else if (!dispatch()) {
                // no more threads
                cell *result;
                if (!list_pop(&re.stack, &result)) {
                    result = cell_void();
                }
                if (re.stack) {
                    cell_unref(error_rt1("stack imbalance", re.stack)); // TODO should this happen?
                }
                run_environment = re.save;
                return result;
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
                    val = error_rt1("no such local", cell_ref(sym));
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

                case c_CFUNR:
                    {
                        void (*def)(cell *) = fun->_.cfunR.def;
                        cell_unref(fun);
                        advance_prog(&re);
                        (*def)(cell_list(arg, NIL));
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

                case c_CFUNR:
                    {
                        void (*def)(cell *) = fun->_.cfunR.def;
                        cell_unref(fun);
                        advance_prog(&re);
                        (*def)(cell_list(arg1,cell_list(arg2,NIL)));
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

                case c_CFUNR:
                    {
                        void (*def)(cell *) = fun->_.cfunR.def;
                        cell_unref(fun);
                        advance_prog_where(re.prog->_.calln.cdr, &re);
                        (*def)(args);
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

                case c_CFUNR:
                    {
                        void (*def)(cell *) = fun->_.cfunR.def;
                        cell_unref(fun);
                        advance_prog(&re);
                        (*def)(tailarg);
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

