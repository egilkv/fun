/*  TAB-P
 *
 *  support for C language bindings in modules
 */

#include "cell.h"

extern cell *hash_f;
extern cell *hash_t;
extern cell *hash_void;
extern cell *hash_undefined;

void arg0(cell *args);
int arg1(cell *args, cell **ap);
int arg2(cell *args, cell **ap, cell **bp);
int arg3(cell *args, cell **ap, cell **bp, cell **cp);
int get_integer(cell *a, integer_t *valuep, cell *dump);
int get_index(cell *a, index_t *indexp, cell *dump);
int get_string(cell *a, char_t **valuep, index_t *lengthp, cell *dump);
int get_cstring(cell *a, char **valuep, cell *dump);
int get_symbol(cell *a, char_t **valuep, cell *dump);
int get_boolean(cell *a, int *boolp, cell *dump);
cell *ref_index(cell *a, index_t index);
