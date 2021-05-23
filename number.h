/*  TAB-P
 *
 */

void make_float(number *np);
int sync_float(number *n1, number *n2);
void normalize_q(number *np);

#define FORMAT_REAL_LEN 24
void format_real(real_t r, char *buf);
