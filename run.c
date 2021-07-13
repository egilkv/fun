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
#include "lock.h"
#include "debug.h"

// statically allocated run environment
// TODO should be same as struct proc_run_env
struct current_run_env {
    cell *prog;             // program being run TODO also in env, but here for efficiency
    cell *env;              // current environment
    cell *stack;            // current runtime stack
} ;

// the one run environment currently being executed
static struct current_run_env *run_environment = 0;
static lock_t run_environment_lock = 0;

// list of other ready processes
struct proc_run_env *ready_list = 0;
lock_t ready_list_lock = 0;

// prepare a new level of compile environment
static struct proc_run_env *proc_run_env_new(cell *prog, cell *env, cell *stack) {
    struct proc_run_env *newenv = malloc(sizeof(struct proc_run_env));
    assert(newenv);
    newenv->prog = prog;
    newenv->env = env;
    newenv->stack = stack;
    newenv->next = NULL;
    return newenv;
}

// free up all run environment resources
void proc_run_env_drop(struct proc_run_env *rep) {
    while (rep) {
        struct proc_run_env *next;
        cell_unref(rep->prog);
        cell_unref(rep->env);
        cell_unref(rep->stack);
        next = rep->next;
        free((void *)rep);
        rep = next;
    }
}

// sweep run environment resources
void proc_run_env_sweep(struct proc_run_env *rep) {
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
    // TODO LOCK(run_environment_lock) ?
    if (!run_environment) {
        cell_unref(error_rt0("breakpoint without run")); // TODO should not happen
        return NIL;
    } else {
        // prevenv and prog are both NIL
        return run_environment->env;
    }
}

// for garbage collect
void sweep_current_run_env() {
    if (run_environment) {
        // TODO use proc_run_env_sweep(run_environment)
        cell_sweep(run_environment->prog);
        cell_sweep(run_environment->env);
        cell_sweep(run_environment->stack);
    }
    // don't forget ready list
    proc_run_env_sweep(ready_list);
}

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
    // TODO LOCK(run_environment_lock)?
    push_value(val, run_environment);
}

static void run_apply(cell *lambda, cell *args, cell *contenv) {
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

    // next program pointer, i.e. return address
    cell *next = cell_ref(run_environment->prog);

    // save current program pointer in environment
#if 1 // TODO tail call enable

    if (run_environment->env != NIL && next == NIL) {
        // tail end call, can drop previous environment:
        // TODO this may be quicker but dirtier:
        //   env_prev(run_environment->env)
        //   env_cont_env(run_environment->env)
        //   env_prog(run_environment->env)
        //   env_assoc(run_environment->env)

        cell *newenv = cell_env(cell_ref(env_prev(run_environment->env)), cell_ref(env_prog(run_environment->env)), newassoc, contenv);

        cell_unref(run_environment->env);
        run_environment->env = newenv;
    } else
#endif
    {
        // push one level of environment
        cell *newenv = cell_env(run_environment->env, next, newassoc, contenv);
        run_environment->env = newenv;
    }
    // switch to new program
    cell_unref(run_environment->prog);
    run_environment->prog = body;
}

// set run_environment to next ready process
// assume current run_environment is NIL
// can be called following a suspend()
static void run_environment_next_ready() {
    struct proc_run_env *newp;

    // get the thing first on the ready list
    LOCK(ready_list_lock);
      if ((newp = ready_list) != NULL) {
          ready_list = newp->next;
      }
    UNLOCK(ready_list_lock);

    assert(run_environment != NULL);
    assert(run_environment->prog == NIL && run_environment->env == NIL);

    if (newp != NULL) {
        newp->next = NULL;
        LOCK(run_environment_lock);
          run_environment->prog = newp->prog;
          run_environment->env = newp->env;
          run_environment->stack = newp->stack;
        UNLOCK(run_environment_lock);
        free(newp);
    }
}

// suspend running process, return its descriptor
// note: run_main will subsequently pick processes of the ready list
struct proc_run_env *suspend() {
    struct proc_run_env *susp;
    // TODO LOCK(run_environment_lock);
    if (run_environment == NULL) {
        assert(0);
    }
    susp = proc_run_env_new(run_environment->prog, run_environment->env, run_environment->stack);

    run_environment->prog = NIL; 
    run_environment->env = NIL;
    run_environment->stack = NIL;

    return susp;
}

// start a new process
// the priority is undefined
void start_process(cell *prog, cell *env, cell *stack) {
    struct proc_run_env *newp;
    newp = proc_run_env_new(prog, env, stack);

    LOCK(ready_list_lock);
      prepend_proc_list(&ready_list, newp);
    UNLOCK(ready_list_lock);

    // TODO should we perhaps dispatch() at this point?
}

// dispatch, rotate ready list
static int dispatch() {
    if (ready_list == NULL) return 0;

    // TODO UNLOCK(run_environment_lock);
    if (run_environment->prog != NIL 
     || run_environment->env != NIL) {
        // there is something of interest currently running, so suspend it
        struct proc_run_env *prev = suspend();
        run_environment_next_ready(); // TODO not really necessary

        LOCK(ready_list_lock);
          append_proc_list(&ready_list, prev);
        UNLOCK(ready_list_lock);

    } else {
        // pop from top of ready list
        run_environment_next_ready();
    }
    return 1;
}

// main run program facility, force running untill evaluated
// TODO temporary kludge
void run_main_force(cell *prog, cell *env0, cell *stack) {
    struct current_run_env *save;

    // TODO needs some locking too
    LOCK(run_environment_lock);
    save = run_environment;
    run_environment = NULL;
    UNLOCK(run_environment_lock);

    // this will then be the run_main for this task
    run_main(prog, env0, stack);

    LOCK(run_environment_lock);
    run_environment = save;
    UNLOCK(run_environment_lock);
}

static void docall2(struct current_run_env *rep, cell *fun, cell *arg1, cell *arg2);
static void docallN(struct current_run_env *rep, cell *fun, cell *args);

// call with 1 argument
static void docall1(struct current_run_env *rep, cell *fun, cell *arg) {
    switch (fun ? fun->type : c_LIST) {
    case c_CFUN1:
        {
            cell *(*def)(cell *) = fun->_.cfun1.def;
            cell **stackp = &(rep->stack);
            cell *result;
            cell_unref(fun);
            advance_prog(rep);
            result = (*def)(arg);
            push_value_stackp(result, stackp); // use stackp in case re changed
        }
        break;

    case c_CFUNN:
        {
            cell *(*def)(cell *) = fun->_.cfun1.def;
            cell **stackp = &(rep->stack);
            cell *result;
            cell_unref(fun);
            advance_prog(rep);
            result = (*def)(cell_list(arg, NIL));
            push_value_stackp(result, stackp);
        }
        break;

    case c_CFUNR:
        {
            void (*def)(cell *) = fun->_.cfunR.def;
            cell_unref(fun);
            advance_prog(rep);
            (*def)(cell_list(arg, NIL));
        }
        break;

    case c_CLOSURE:
        {
            cell *lambda = cell_ref(fun->_.cons.car);
            cell *contenv = cell_ref(fun->_.cons.cdr);
            cell_unref(fun);
            advance_prog(rep);
            run_apply(lambda, cell_list(arg, NIL), contenv);
        }
        break;
    case c_CLOSURE0:
    case c_CLOSURE0T:
        advance_prog(rep);
        run_apply(fun, cell_list(arg, NIL), NIL);
        break;

    case c_BIND:
        {
            cell *fun0 = cell_ref(fun->_.cons.cdr);
            cell *arg0 = cell_ref(fun->_.cons.car);
            cell_unref(fun);
            docall2(rep, fun0, arg0, arg);
        }
        break;

    default:
        cell_unref(arg);
        push_value(error_rt1("not a function with 1 arg", fun), rep);
        advance_prog(rep);
        break;
    }
}

// call with 2 arguments
static void docall2(struct current_run_env *rep, cell *fun, cell *arg1, cell *arg2) {
    switch (fun ? fun->type : c_LIST) {
    case c_CFUN2:
        {
            cell *(*def)(cell *, cell *) = fun->_.cfun2.def;
            cell **stackp = &(rep->stack);
            cell *result;
            cell_unref(fun);
            advance_prog(rep);
            result = (*def)(arg1, arg2);
            push_value_stackp(result, stackp);
        }
        break;

    case c_CFUNN:
        {
            cell *(*def)(cell *) = fun->_.cfun1.def;
            cell **stackp = &(rep->stack);
            cell *result;
            cell_unref(fun);
            advance_prog(rep);
            result = (*def)(cell_list(arg1,cell_list(arg2,NIL)));
            push_value_stackp(result, stackp);
        }
        break;

    case c_CFUNR:
        {
            void (*def)(cell *) = fun->_.cfunR.def;
            cell_unref(fun);
            advance_prog(rep);
            (*def)(cell_list(arg1,cell_list(arg2,NIL)));
        }
        break;

    case c_CLOSURE:
        {
            cell *lambda = cell_ref(fun->_.cons.car);
            cell *contenv = cell_ref(fun->_.cons.cdr);
            cell_unref(fun);
            advance_prog(rep);
            run_apply(lambda, cell_list(arg1,cell_list(arg2, NIL)), contenv);
        }
        break;

    case c_CLOSURE0:
    case c_CLOSURE0T:
        advance_prog(rep);
        run_apply(fun, cell_list(arg1,cell_list(arg2, NIL)), NIL);
        break;

    case c_BIND:
        {
            cell *fun0 = cell_ref(fun->_.cons.cdr);
            cell *arg0 = cell_ref(fun->_.cons.car);
            cell_unref(fun);
            docallN(rep, fun0,
                cell_list(arg0, cell_list(arg1, cell_list(arg2, NIL))));
        }
        break;

    default:
        cell_unref(arg1);
        cell_unref(arg2);
        push_value(error_rt1("not a function with 2 args", fun), rep);
        advance_prog(rep);
        break;
    }
}

// call with N arguments
static void docallN(struct current_run_env *rep, cell *fun, cell *args) {
    switch (fun ? fun->type : c_LIST) {
    case c_CFUNN:
        {
            cell *(*def)(cell *) = fun->_.cfun1.def;
            cell **stackp = &(rep->stack);
            cell *result;
            cell_unref(fun);
            // TODO there is also another OP using calln
            advance_prog_where(rep->prog->_.calln.cdr, rep);
            result = (*def)(args);
            push_value_stackp(result, stackp);
        }
        break;

    case c_CFUNR:
        {
            void (*def)(cell *) = fun->_.cfunR.def;
            cell_unref(fun);
            advance_prog_where(rep->prog->_.calln.cdr, rep);
            (*def)(args);
        }
        break;

    case c_CLOSURE:
        {
            cell *lambda = cell_ref(fun->_.cons.car);
            cell *contenv = cell_ref(fun->_.cons.cdr);
            cell_unref(fun);
            advance_prog(rep);
            run_apply(lambda, args, contenv);
        }
        break;

    case c_CLOSURE0:
    case c_CLOSURE0T:
        advance_prog(rep);
        run_apply(fun, args, NIL);
        break;

    case c_BIND:
        {
            cell *fun0 = cell_ref(fun->_.cons.cdr);
            cell *arg0 = cell_ref(fun->_.cons.car);
            cell_unref(fun);
            if (args == NIL) {
                docall1(rep, fun0, arg0);
            } else if (cell_cdr(args) == NIL) { // probably cannot be 1 arg
                cell *arg1 = NIL;
                list_pop(&args, &arg1);
                assert(args == NIL);
                docall2(rep, fun0, arg0, arg1);
            } else {
                docallN(rep, fun0, cell_list(arg0, args));
            }
        }
        break;

    default:
        cell_unref(args);
        push_value(error_rt1("not a function", fun), rep);
        advance_prog_where(rep->prog->_.calln.cdr, rep);
        break;
    }
}

// main run program facility, return result
void run_main(cell *prog, cell *env0, cell *stack) {
    struct current_run_env re;

    LOCK(run_environment_lock);
    if (run_environment) {
        // already running?
        // TODO this is still not 100% safe
        struct proc_run_env *pre = proc_run_env_new(prog, env0, stack);
        // TODO dual locks, check if we can have deadlocks

        LOCK(ready_list_lock);
          prepend_proc_list(&ready_list, pre);
        UNLOCK(ready_list_lock);

        UNLOCK(run_environment_lock);
        return;
    }
    re.prog = prog;
    re.env = env0;
    re.stack = stack;
    run_environment = &re;
    UNLOCK(run_environment_lock);

    for (;;) {
        while (re.prog == NIL) {
            if (re.env != NIL) {
                // pop one level of program/environment stack
                cell *contprog = cell_ref(env_prog(re.env));
                cell *prevenv = cell_ref(env_prev(re.env));
                cell_unref(re.env);
                re.env = prevenv;
                re.prog = contprog;
            } else {
                // a thread stopped
                assert(re.prog == NIL && re.env == NIL);
                cell_unref(re.stack); // throw away any results
                re.stack = NIL;
                if (!dispatch()) { // TODO logic is shaky
                    //fprintf(stdout, "*no more threads*\n"); // TODO only in debug mode something
                    LOCK(run_environment_lock);
                      run_environment = NULL;
                    UNLOCK(run_environment_lock);
                    return;
                } else {
                    fprintf(stdout, "*thread stopped*\n"); // TODO only in debug mode something
                }
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
                cell *fun = re.prog->_.cons.car ? cell_ref(re.prog->_.cons.car) : pop_value(&re);
                cell *arg = pop_value(&re);
                docall1(&re, fun, arg);
            }
            break;

        case c_DOCALL2:   // car is closure or function, pop 2 args, push result
            // function evaluated on stack?
            {
                cell *fun = re.prog->_.cons.car ? cell_ref(re.prog->_.cons.car) : pop_value(&re);
                cell *arg1 = pop_value(&re);
                cell *arg2 = pop_value(&re);
                docall2(&re, fun, arg1, arg2);
            }
            break;

        case c_DOCALLN:   // car is Nargs, pop function, pop N args, push result
            {
                integer_t narg = re.prog->_.calln.narg;
                cell *args = NIL;
                cell **pp = &args;
                cell *fun = pop_value(&re);
                // function evaluated on stack
                while (narg-- > 0) {
                    cell *a = pop_value(&re);
                    *pp = cell_list(a, NIL);
                    pp = &((*pp)->_.cons.cdr);
                }
                docallN(&re, fun, args);
            }
            break;

        case c_DOCOND:    // pop, car if true, cdr else
            {
                int bool;
                cell *cond = pop_value(&re);
                if (!get_boolean(cond, &bool, NIL)) {
                    bool = 0; // assume false, sensible choice for && and ||
                }
                if (bool) {
                    advance_prog_where(re.prog->_.cons.car, &re);
                } else {
                    advance_prog(&re);
                }
            }
            break;

        case c_DOIF:      // pop, car if true, cdr is c_DOELSE
            {
                int bool;
                cell *cond = pop_value(&re);
                if (!get_boolean(cond, &bool, NIL)) {
                    // error condition, continue with void value
                    // TODO use error value above instead...
                    push_value(cell_void(), &re);
                    advance_prog(&re);
                    assert(re.prog && re.prog->type == c_DOELSE);
                    advance_prog(&re);

                } else if (bool) {
                    advance_prog_where(re.prog->_.cons.car, &re); // go calculate if-true value

                } else {
                    // follow else-path
                    advance_prog(&re);
                    assert(re.prog && re.prog->type == c_DOELSE);
                    advance_prog_where(re.prog->_.cons.car, &re); // go calculate if-false value
                }
            }
            break;

        case c_DOELSE:    // cdr is next
            // when encountered outright, it is following push of either if-true or if-false value
            advance_prog(&re);
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
                        run_apply(lambda, tailarg, contenv);
                    }
                    break;
                case c_CLOSURE0:
                case c_CLOSURE0T:
                    advance_prog(&re);
                    run_apply(fun, tailarg, NIL);
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

