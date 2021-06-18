/*  TAB-P
 *
 */

struct run_env {
    cell *prog;             // program being run TODO also in env, but here for efficiency
    cell *stack;            // current runtime stack
    cell *env;              // current environment
    struct run_env *save;   // previous runtime environments, used when debugging primarily
    struct run_env *next;   // next in line, for ready_list and such
} ;

void run_environment_drop(struct run_env *rep);
void run_environment_sweep(struct run_env *rep);

// only one invokation:
cell *run_main(cell *prog, cell *env0);

void run_main_apply(cell *lambda, cell *args);

void run_async(cell *prog);

// for debugging
cell *current_run_env();
