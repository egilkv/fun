/*  TAB-P
 */

#include "cell.h"

void oblist_init();

cell *oblistv(const char *sym, cell *val);
cell *oblista(char *sym);

void oblist_set(cell *sym, cell *val);

char *oblist_search(const char *lookfor, int state);

integer_t oblist_sweep();

extern int oblist_teardown; // TODO for assert only

