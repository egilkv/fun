//  TAB-P
//
//  functions to support debugging
//

void debug_prints(const char *msg);
void debug_write(cell *ct);
void debug_writeln(cell *ct);
void debug_trace(cell *a);
int debug_traceon(cell *a);
int debug_traceoff(cell *a);

void debug_init();

