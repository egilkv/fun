/* TAB P
 *
 */

#include "cell.h"

extern cell *hash_defq;
extern cell *hash_div;
extern cell *hash_eval;
extern cell *hash_f;
extern cell *hash_gt;
extern cell *hash_minus;
extern cell *hash_not;
extern cell *hash_plus;
extern cell *hash_quote;
extern cell *hash_t;
extern cell *hash_times;
extern cell *hash_void;

cell *cfun_eval(cell *a);

void cfun_init();
