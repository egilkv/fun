/*  TAB-P
 *
 */

#include "cell.h"

int get_float(cell *a, number *np, cell *dump);

cell *cell_integer(integer_t integer);
cell *cell_real(real_t real);

void make_float(number *np);
int sync_float(number *n1, number *n2);
void normalize_q(number *np);
int make_negative(number *np);

#define FORMAT_REAL_LEN 24
void format_real(real_t r, char *buf);

cell *err_overflow(cell *dump);
