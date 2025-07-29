/*  TAB-P
 *
 */

#include "cell.h"
#include "cmod.h"

#ifndef EXTDEF
#define EXTDEF extern
#endif

EXTDEF cell *hash_args;
EXTDEF cell *hash_assoc;
EXTDEF cell *hash_bind;
EXTDEF cell *hash_cat;
EXTDEF cell *hash_channel;
EXTDEF cell *hash_do;
EXTDEF cell *hash_eq;
EXTDEF cell *hash_ge;
EXTDEF cell *hash_go;
EXTDEF cell *hash_gt;
EXTDEF cell *hash_le;
EXTDEF cell *hash_lt;
EXTDEF cell *hash_list;
EXTDEF cell *hash_minus;
EXTDEF cell *hash_not;
EXTDEF cell *hash_noteq;
EXTDEF cell *hash_plus;
EXTDEF cell *hash_quotient;
EXTDEF cell *hash_receive;
EXTDEF cell *hash_result;
EXTDEF cell *hash_ref;
EXTDEF cell *hash_send;
EXTDEF cell *hash_times;

EXTDEF cell *hash_stdin;
EXTDEF cell *hash_stdout;

void cfun_init();
void cfun_args(int argc, char * const argv[]);
