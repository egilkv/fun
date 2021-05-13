
#include "cell.h"

cell *oblistv(const char *sym, cell *val);
cell *oblista(char *sym);
cell *oblists(const char *sym);

void oblist_set(cell *sym, cell *val);

void oblist_drop(int show);
