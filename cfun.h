/*  TAB-P
 *
 */

#include "cell.h"
#include "cmod.h"

cell *hash_and;
cell *hash_args;
cell *hash_assoc;
cell *hash_cat;
cell *hash_defq;
cell *hash_do;
cell *hash_eq;
cell *hash_ge;
cell *hash_gt;
cell *hash_if;
cell *hash_lambda;
cell *hash_le;
cell *hash_lt;
cell *hash_minus;
cell *hash_not;
cell *hash_noteq;
cell *hash_or;
cell *hash_plus;
cell *hash_quote;
cell *hash_quotient;
cell *hash_ref;
cell *hash_refq;
cell *hash_times;
cell *hash_vector;

void cfun_init();
void cfun_args(int argc, char * const argv[]);
