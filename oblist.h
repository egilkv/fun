/*  TAB-P
 *
 */

#include "cell.h"

void oblist_init();

cell *symbol_set(const char *sym, cell *val);
cell *asymbol_find(char *sym);

cell *symbol_self(const char *sym);
cell *symbol_peek(const char *sym);

void oblist_set(cell *sym, cell *val);

char *oblist_search(const char *lookfor, int state);

integer_t oblist_sweep();

extern int oblist_teardown; // TODO for assert only

