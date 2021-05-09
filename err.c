/*  TAB P
 *
 *  error handling
 */

#include <stdio.h>
#include <ctype.h>

#include "cfun.h" // hash_void
#include "err.h"
#include "io.h"

// runtime error, 0 arguments
// arg is consumed, return void
cell *error_rt0(const char *msg) {
    fprintf(stderr,"error; %s\n", msg);
    return cell_ref(hash_void); // error
}

// runtime error, 1 numeric argument
cell *error_rti(const char *msg, integer_t val) {
    fprintf(stderr,"error; %s: %ld\n", msg, val);
    return cell_ref(hash_void); // error
}

// runtime error, 1 argument
// arg is consumed, return void
cell *error_rt1(const char *msg, cell *arg) {
    fprintf(stderr,"error; %s: ", msg);
    cell_print(stderr, arg);
    fprintf(stderr,"\n");
    cell_unref(arg);
    return cell_ref(hash_void); // error
}

// parsing error
void error_par(const char *msg) {
    fprintf(stderr,"error; %s\n", msg);
}

// parsing error, with type
void error_pat(const char *msg, int type) {
    // TODO improve
    fprintf(stderr,"error; %s: type is %d\n", msg, type);
}

// lexical error, show character or -1 if none
void error_lex(const char *msg, int c) {
    fprintf(stderr,"error; %s", msg);
    if (c >= 0) {
	if (iscntrl(c)) {
	    fprintf(stderr,": 0x%02x", c);
	} else if (c == '"') {
	    fprintf(stderr, ": \"\\\"\"");
	} else if (c == '\\') {
	    fprintf(stderr, ": \"\\\\\"");
	} else {
	    fprintf(stderr, ": \"%c\"", c);
	}
    }
    fprintf(stderr,"\n");
}


