/*  TAB-P
 *
 *  error handling
 */

#include <stdio.h>
#include <ctype.h>

#include "cmod.h"
#include "err.h"
#include "m_io.h"

static const char *it_name[] = {
   "'",         // it_QUOTE,   // 0
   "+",         // it_PLUS,
   "-",         // it_MINUS,
   "*",         // it_MULT,
   "/",         // it_DIV,
   "<",         // it_LT,      // 5
   "!",         // it_NOT,
   ">",         // it_GT,
   "=",         // it_EQ,
   "<=",        // it_LTEQ,
   ">=",        // it_GTEQ,    // 10
   "!=",        // it_NTEQ,
   "==",        // it_EQEQ,
   "&",         // it_AMP,
   "&&",        // it_AND,
   ".",         // it_STOP,    // 15
   "..",        // it_RANGE,
   ",",         // it_COMMA,
   ".",         // it_COLON,
   ";",         // it_SEMI,
   "?",         // it_QUEST,   // 20
   "|",         // it_BAR,
   "||",        // it_OR,
   "++",        // it_CAT,
   "^",         // it_CIRC,
   "~",         // it_TILDE,   // 25
   "...",       // it_ELLIP,
   "(",         // it_LPAR,
   ")",         // it_RPAR,
   "[",         // it_LBRK,
   "]",         // it_RBRK,    // 30
   "{",         // it_LBRC,
   "}",         // it_RBRC,
   "number",    // it_NUMBER,
   "string",    // it_STRING,
   "symbol"     // it_SYMBOL
} ;

// runtime error, 0 arguments
// arg is consumed, return void
cell *error_rt0(const char *msg) {
    fflush(stdout);
    fprintf(stderr,"error; %s\n", msg);
    fflush(stderr);
    return cell_error();
}

// runtime error, 1 numeric argument
cell *error_rti(const char *msg, integer_t val) {
    fflush(stdout);
    fprintf(stderr,"error; %s: %lld\n", msg, val);
    fflush(stderr);
    return cell_error();
}

// runtime error, 1 string argument
cell *error_rts(const char *msg, const char *info) {
    fflush(stdout);
    fprintf(stderr,"error; %s: %s\n", msg, info);
    fflush(stderr);
    return cell_error();
}

// runtime error, no argument
cell *error_rt(const char *msg) {
    fflush(stdout);
    fprintf(stderr,"error; %s\n", msg);
    fflush(stderr);
    return cell_error();
}

// runtime error, 1 argument
// arg is consumed, return void
cell *error_rt1(const char *msg, cell *arg) {
    fflush(stdout);
    fprintf(stderr,"error; %s: ", msg);
    cell_write(stderr, arg);
    fprintf(stderr,"\n");
    fflush(stderr);
    cell_unref(arg);
    return cell_error();
}

// parsing error
void error_par(const char *info, const char *msg) {
    fflush(stdout);
    fprintf(stderr,"error%s; %s\n", info, msg);
    fflush(stderr);
}

// parsing error, with type
void error_pat(const char *info, const char *msg, int type) {
    fflush(stdout);
    // TODO make size type is within range and treat symbols strings numbers differently
    fprintf(stderr,"error%s; %s: operator is \"%s\"\n", info, msg, it_name[type]);
    fflush(stderr);
}

// parsing error, 1 argument
// arg is consumed
void error_pa1(const char *info, const char *msg, cell *arg) {
    fflush(stdout);
    fprintf(stderr,"error%s; %s: ", info, msg);
    cell_write(stderr, arg);
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
void error_lex(const char *info, const char *msg, int c) {
    fflush(stdout);
    fprintf(stderr,"error%s; %s", info, msg);
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


