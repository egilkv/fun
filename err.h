/* TAB P
 */

#include "cell.h"

cell *error_rt0(const char *msg);
cell *error_rti(const char *msg, integer_t val);
cell *error_rt1(const char *msg, cell *arg);

void error_par(const char *msg);
void error_pat(const char *msg, int type);
void error_pa1(const char *msg, cell *arg);

void error_lex(const char *msg, int c);

void error_cmdopt(const char *msg, int c);
void error_cmdstr(const char *msg, const char *s);
