/*  TAB-P
 *
 *  support for C language bindings in modules
 */

#include "cell.h"

extern cell *hash_f;
extern cell *hash_t;
extern cell *hash_void;
extern cell *hash_undef;
extern cell *hash_ellip;

int at_least_one(cell **argsp, cell **argp);
void arg0(cell *args);
int arg1(cell *args, cell **ap);
int arg2(cell *args, cell **ap, cell **bp);
int arg3(cell *args, cell **ap, cell **bp, cell **cp);
int peek_number(cell *a, number *np, cell *dump);
int get_number(cell *a, number *valuep, cell *dump);
int get_integer(cell *a, integer_t *valuep, cell *dump);
int get_any_integer(cell *a, integer_t *valuep, cell *dump);
int get_index(cell *a, index_t *indexp, cell *dump);
int peek_string(cell *a, char_t **valuep, index_t *lengthp, cell *dump);
int peek_cstring(cell *a, char **valuep, cell *dump);
int peek_symbol(cell *a, char_t **valuep, cell *dump);
int get_symbol(cell *a, char_t **valuep, cell *dump);
int peek_boolean(cell *a, int *boolp);
int get_boolean(cell *a, int *boolp, cell *dump);
int peek_special(const char *magic, cell *arg, void **valuep, cell *dump);
int get_special(const char *magic, cell *arg, void **valuep, cell *dump);
integer_t ref_length(cell *a);
cell *ref_index(cell *a, index_t index);
cell *ref_range1(cell *a, index_t index);
cell *ref_range2(cell *a, index_t index, integer_t len);
cell *cell_void();
cell *cell_error();

cell *cell_boolean(int bool);

cell *cfun2_ref(cell *a, cell *b);

int exists_on_list(cell *list, cell *item);

cell *defq(cell *nam, cell *val, cell **envp);

