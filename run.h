/*  TAB-P
 *
 */

// only one invokation:
cell *run_main(cell *prog, cell *env0);

void run_main_apply(cell *lambda, cell *args);

void run_async(cell *prog);

// for debugging
cell *current_run_env();
