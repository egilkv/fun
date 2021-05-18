/* TAB P
 *
 */

#include "cell.h"
#include "cmod.h"

extern cell *hash_amp;
extern cell *hash_args;
extern cell *hash_assoc;
extern cell *hash_defq;
extern cell *hash_div;
extern cell *hash_do;
extern cell *hash_eq;
extern cell *hash_if;
extern cell *hash_lambda;
extern cell *hash_list;
extern cell *hash_lt;
extern cell *hash_minus;
extern cell *hash_not;
extern cell *hash_noteq;
extern cell *hash_plus;
extern cell *hash_quote;
extern cell *hash_ref;
extern cell *hash_refq;
extern cell *hash_times;
extern cell *hash_vector;

void cfun_init();
void cfun_args(int argc, char * const argv[]);
void cfun_drop();
