/* TAB P
 *
 */

#include "cell.h"

extern cell *hash_amp;
extern cell *hash_args;
extern cell *hash_assoc;
extern cell *hash_defq;
extern cell *hash_div;
extern cell *hash_do;
extern cell *hash_f;
extern cell *hash_if;
extern cell *hash_lambda;
extern cell *hash_list;
extern cell *hash_lt;
extern cell *hash_minus;
extern cell *hash_not;
extern cell *hash_plus;
extern cell *hash_quote;
extern cell *hash_ref;
extern cell *hash_refq;
extern cell *hash_t;
extern cell *hash_times;
extern cell *hash_use;
extern cell *hash_vector;
extern cell *hash_void;

void arg0(cell *args);
int arg1(cell *args, cell **ap);
int arg2(cell *args, cell **ap, cell **bp);
int arg3(cell *args, cell **ap, cell **bp, cell **cp);
int get_integer(cell *a, integer_t *valuep, cell *dump);
int get_index(cell *a, index_t *indexp, cell *dump);
int get_string(cell *a, char_t **valuep, index_t *lengthp, cell *dump);
int get_cstring(cell *a, char **valuep, cell *dump);
int get_symbol(cell *a, char_t **valuep, cell *dump);
cell *ref_index(cell *a, index_t index);

void cfun_init();
void cfun_drop();
