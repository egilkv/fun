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
    fflush(stdout);
    fprintf(stderr,"error; %s\n", msg);
    fflush(stderr);
    return cell_ref(hash_void); // error
}

// runtime error, 1 numeric argument
cell *error_rti(const char *msg, integer_t val) {
    fflush(stdout);
    fprintf(stderr,"error; %s: %lld\n", msg, val);
    fflush(stderr);
    return cell_ref(hash_void); // error
}

// runtime error, 1 argument
// arg is consumed, return void
cell *error_rt1(const char *msg, cell *arg) {
    fflush(stdout);
    fprintf(stderr,"error; %s: ", msg);
    cell_print(stderr, arg);
    fprintf(stderr,"\n");
    fflush(stderr);
    cell_unref(arg);
    return cell_ref(hash_void); // error
}

// parsing error
void error_par(const char *msg) {
    fflush(stdout);
    fprintf(stderr,"error; %s\n", msg);
    fflush(stderr);
}

// parsing error, with type
void error_pat(const char *msg, int type) {
    fflush(stdout);
    // TODO improve
    fprintf(stderr,"error; %s: type is %d\n", msg, type);
    fflush(stderr);
}

// parsing error, 1 argument
// arg is consumed
void error_pa1(const char *msg, cell *arg) {
    fflush(stdout);
    fprintf(stderr,"error; %s: ", msg);
    cell_print(stderr, arg);
    fprintf(stderr,"\n");
    fflush(stderr);
    cell_unref(arg);
}

static void printable_c(int c) {
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
}

// lexical error, show character or -1 if none
void error_lex(const char *msg, int c) {
    fflush(stdout);
    fprintf(stderr,"error; %s", msg);
    printable_c(c);
    fprintf(stderr,"\n");
    fflush(stderr);
}

// command line option error, show character
void error_cmdopt(const char *msg, int c) {
    fflush(stdout);
    fprintf(stderr,"error; %s", msg);
    printable_c(c);
    fprintf(stderr,"\n");
    fflush(stderr);
}

// command line option error, show string
void error_cmdstr(const char *msg, const char *s) {
    fflush(stdout);
    fprintf(stderr,"error; %s: %s\n", msg, s);
    fflush(stderr);
}


