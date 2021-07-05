/*  TAB-P
 *
 */

struct proc_run_env {
    cell *prog;             // current program
    cell *env;              // current environment TODO what is prog here?
    cell *stack;            // current runtime stack
    struct proc_run_env *next; // next in line
} ;

extern struct proc_run_env *ready_list;
extern lock_t ready_list_lock;

void proc_run_env_drop(struct proc_run_env *rep);
void proc_run_env_sweep(struct proc_run_env *rep);
void append_proc_list(struct proc_run_env **pp, struct proc_run_env *rep);
void prepend_proc_list(struct proc_run_env **pp, struct proc_run_env *rep);
struct proc_run_env *suspend();

void start_process(cell *prog, cell *env, cell *stack);

void run_main(cell *prog, cell *env0, cell *stack);

void run_main_force(cell *prog, cell *env0, cell *stack);

void push_stack_current_run_env(cell *val);

void sweep_current_run_env();

// for debugging
cell *current_run_env();
