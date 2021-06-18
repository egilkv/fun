/*  TAB-P
 *
 */

struct proc_run_env {
    cell *env;              // current environment, including program pointer
    cell *stack;            // current runtime stack
    int is_main;            // main thread
    struct proc_run_env *next; // next in line
} ;

extern struct proc_run_env *ready_list;

struct proc_run_env *run_environment_new(cell *env, cell *stack);
void run_environment_drop(struct proc_run_env *rep);
void run_environment_sweep(struct proc_run_env *rep);
void append_ready_list(struct proc_run_env *rep);

// only one invokation:
cell *run_main(cell *prog, cell *env0);

void run_main_apply(cell *lambda, cell *args);

void run_async(cell *prog);

// for debugging
cell *current_run_env();
