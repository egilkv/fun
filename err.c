/*  TAB P
 *
 *  error handling
 */

#include <stdio.h>

#include "cfun.h" // hash_void
#include "err.h"

// runtime error, 0 arguments
// arg is consumed, return void
cell *error_rt0(const char *msg) {
    printf("Error; %s\n", msg);
    return cell_ref(hash_void); // error
}

// runtime error, 1 argument
// arg is consumed, return void
cell *error_rt1(const char *msg, cell *arg) {
    printf("Error; %s: ", msg);
    cell_print(arg);
    printf("\n");
    cell_unref(arg);
    return cell_ref(hash_void); // error
}
