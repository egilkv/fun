/*  TAB P
 *
 *  error handling
 */

#include <stdio.h>

#include "cfun.h" // hash_void
#include "err.h"
#include "io.h"

// runtime error, 0 arguments
// arg is consumed, return void
cell *error_rt0(const char *msg) {
    fprintf(stderr,"Error; %s\n", msg);
    return cell_ref(hash_void); // error
}

// runtime error, 1 numeric argument
cell *error_rti(const char *msg, integer_t val) {
    fprintf(stderr,"Error; %s: %ld\n", msg, val);
    return cell_ref(hash_void); // error
}

// runtime error, 1 argument
// arg is consumed, return void
cell *error_rt1(const char *msg, cell *arg) {
    fprintf(stderr,"Error; %s: ", msg);
    cell_print(stderr, arg);
    fprintf(stderr,"\n");
    cell_unref(arg);
    return cell_ref(hash_void); // error
}
