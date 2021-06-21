/*  TAB-P
 *
 */

#include "cell.h"
#include "cmod.h"

cell *hash_args;
cell *hash_assoc;
cell *hash_cat;
cell *hash_channel;
cell *hash_do;
cell *hash_eq;
cell *hash_ge;
cell *hash_go;
cell *hash_gt;
cell *hash_le;
cell *hash_lt;
cell *hash_list;
cell *hash_minus;
cell *hash_not;
cell *hash_noteq;
cell *hash_plus;
cell *hash_quotient;
cell *hash_receive;
cell *hash_ref;
cell *hash_send;
cell *hash_times;

cell *hash_stdin;
cell *hash_stdout;

void cfun_init();
void cfun_args(int argc, char * const argv[]);
