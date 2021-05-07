/* TAB P
 *
 * TODO should evaluation happen in functions? perhaps
 */

#include <stdio.h> // debug
#include <assert.h>

#include "cfun.h"
#include "oblist.h"

cell *hash_defq;
cell *hash_div;
cell *hash_eval;
cell *hash_f;
cell *hash_gt;
cell *hash_if;
cell *hash_minus;
cell *hash_not;
cell *hash_plus;
cell *hash_quote;
cell *hash_t;
cell *hash_times;
cell *hash_void;

// function with 1 argument
static int arg1(cell *args, cell **a1p) {
    if (cell_split(args, a1p, &args)) {
        if (args) {
            printf("Error: Excess argument(s) ignored: "); // TODO error
            cell_print(args);
            printf("\n");
	    cell_unref(args);
        }
        return 1;
    } else {
        *a1p = NIL;
        if (args == NIL) {
            printf("Error: Missing argument\n"); // TODO error
        } else {
            printf("Error: Malformed arguments: "); // TODO error and can it happen?
            cell_print(args);
            printf("\n");
	    cell_unref(args);
        }
        return 0;
    }
}

// function with 2 arguments
static int arg2(cell *args, cell **a1p, cell **a2p) {
    if (cell_split(args, a1p, &args)) {
        if (cell_split(args, a2p, &args)) {
            if (args) {
                printf("Error: Excess argument(s) ignored: "); // TODO error
                cell_print(args);
                printf("\n");
		cell_unref(args);
            }
            return 1;
        }
        *a2p = NIL;
    }
    *a1p = NIL;
    if (args == NIL) {
        printf("Error: Missing argument(s)\n"); // TODO error
    } else {
        printf("Error: Malformed arguments: "); // TODO error and can it happen?
        cell_print(args);
        printf("\n");
	cell_unref(args);
    }
    return 0;
}

// a in always unreffed
// dump is unreffed only if error
static int pick_numeric(cell *a, long int *valuep, cell *dump) {
    if (a) {
        switch (a->type) {
        case c_INTEGER:
            *valuep = a->_.ivalue;
            cell_unref(a);
            return 1;
        }
    }
    printf("Error: Not a number: "); // TODO error message
    cell_print(a);
    printf("\n");
    cell_unref(a);
    cell_unref(dump);
    return 0;
}

// a in always unreffed
// dump is unreffed only if error
static int pick_boolean(cell *a, int *boolp, cell *dump) {
    if (a == hash_t) {
        *boolp = 1;
        cell_unref(a);
        return 1;
    } else if (a == hash_f) {
        *boolp = 0;
        cell_unref(a);
        return 1;
    }
    printf("Error: Not a Boolean: "); // TODO error message
    cell_print(a);
    printf("\n");
    cell_unref(a);
    cell_unref(dump);
    return 0;
}

static int verify_nul(cell *a) {
    if (a) {
        printf("Error: Improper list: "); // TODO error message
	cell_print(a);
	printf("\n");
	cell_unref(a);
	return 0;
    } else {
	return 1;
    }
}

static cell *cfun_defq(cell *args) {
    cell *a, *b;
    if (arg2(args, &a, &b)) {
        if (cell_is_symbol(a)) {
	    cell_unref(a->_.symbol.val);
            b = a->_.symbol.val = cfun_eval(b);
	    cell_unref(a);
            return cell_ref(b);
        } else {
            printf("Error: Not a symbol: "); // TODO error message
            cell_print(a);
            printf("\n");
	    cell_unref(a);
	    cell_unref(b);
        }
    }
    return cell_ref(hash_void); // error
}

static cell *cfun_not(cell *args) {
    cell *a;
    int bool;
    if (arg1(args, &a)) {
	a = cfun_eval(a);
        if (pick_boolean(a, &bool, NIL)) {
	    return bool ? cell_ref(hash_f) : cell_ref(hash_t);
        }
    }
    return cell_ref(hash_void); // error
}

static cell *cfun_if(cell *args) {
    int bool;
    cell *a;
    if (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_boolean(a, &bool, args)) return cell_ref(hash_void); // error
    }
    // second argument must be present
    if (!cell_split(args, &a, &args)) {
        printf("Error: missing body for if statement: "); // TODO error message
        cell_print(args);
        printf("\n");
        cell_unref(args);
        return cell_ref(hash_void); // error
    }
    if (bool) { // true?
        cell_unref(args);
        return cfun_eval(a);
    } 
    // else
    cell_unref(a);
    if (!args) {
        // no else part given, value is void
        return cell_ref(hash_void);
    } else if (!cell_split(args, &a, &args)) {
        printf("Error: missing body for else statement: "); // TODO error message
        cell_print(args);
        printf("\n");
        cell_unref(args);
        return cell_ref(hash_void); // error
    }
    if (args) {
        printf("Error: extra argument for if ignored: "); // TODO error message
        cell_print(args);
        printf("\n");
        cell_unref(args);
    }
    // finally, else proper
    return cfun_eval(a);
}

static cell *cfun_quote(cell *args) {
    cell *a;
    if (arg1(args, &a)) {
	return a;
    }
    return cell_ref(hash_void); // error
}

static cell *cfun_plus(cell *args) {
    long int result = 0;
    long int operand;
    cell *a;
    while (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_numeric(a, &operand, args)) return cell_ref(hash_void); // error
        result += operand; // TODO overflow etc
    }
    if (!verify_nul(args)) return cell_ref(hash_void); // error
    return cell_integer(result);
}

static cell *cfun_minus(cell *args) {
    long int result = 0;
    long int operand;
    cell *a;
    if (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_numeric(a, &result, args)) return cell_ref(hash_void); // error
	if (args == NIL) {
            // special case, one argument
            result = -result; // TODO overflow
        }
    }
    while (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_numeric(a, &operand, args)) return cell_ref(hash_void); // error
        result -= operand; // TODO overflow etc
    }
    if (!verify_nul(args)) return cell_ref(hash_void); // error
    return cell_integer(result);
}

static cell *cfun_times(cell *args) {
    long int result = 1;
    long int operand;
    cell *a;
    while (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_numeric(a, &operand, args)) return cell_ref(hash_void); // error
        result *= operand; // TODO overflow etc
    }
    if (!verify_nul(args)) return cell_ref(hash_void); // error
    return cell_integer(result);
}

static cell *cfun_div(cell *args) {
    long int result = 0;
    long int operand;
    cell *a;
    // TODO must have two arguments?
    if (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_numeric(a, &result, args)) return cell_ref(hash_void); // error
    }
    while (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_numeric(a, &operand, args)) return cell_ref(hash_void); // error
        if (operand == 0) {
            printf("Error: Division by zero\n"); // TODO error message
            cell_unref(args);
            return cell_ref(hash_void); // error
        }
        // TODO should rather create a quotient
        result /= operand;
    }
    if (!verify_nul(args)) return cell_ref(hash_void); // error
    return cell_integer(result);
}

static cell *cfun_gt(cell *args) {
    long int value;
    long int operand;
    cell *a;
    if (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_numeric(a, &value, args)) return cell_ref(hash_void); // error
    }
    while (cell_split(args, &a, &args)) {
	a = cfun_eval(a);
        if (!pick_numeric(a, &operand, args)) return cell_ref(hash_void); // error
        if (value > operand) { // condition satisfied?
            value = operand;
	} else {
	    cell_unref(args);
	    return cell_ref(hash_f); // false
	}
    }
    if (!verify_nul(args)) return cell_ref(hash_void); // error
    return cell_ref(hash_t);
}

cell *cfun_eval(cell *a) {
    if (a) switch (a->type) {
    case c_INTEGER: // value is itself
    case c_STRING:
    case c_CFUN:    // TODO maybe #void
        return a;

    case c_SYMBOL:  // evaluate symbol
	return cell_ref(a->_.symbol.val);

    case c_CONS:    
        {
            cell *fun;
            cell *arg;
            struct cell_s *(*def)(struct cell_s *);
	    cell_split(a, &fun, &arg);
            fun = cfun_eval(fun);
	    // TODO may consider moving evaluation out of functions

	    switch (fun ? fun->type : c_CONS) {
            case c_CFUN:
                def = fun->_.cfun.def;
                cell_unref(fun);
                return (*def)(arg);
            case c_INTEGER: // not a function
            case c_STRING:
            case c_SYMBOL:
            case c_CONS:
		printf("Error: Not a function: "); // TODO error
		cell_print(fun);
		printf("\n");
		cell_unref(fun);
		cell_unref(arg);
		return cell_ref(hash_void); // error
            default:
                assert(0);
	    }
        }
    default:
        assert(0);
    }
    printf("#eval: ");
    cell_print(a);
    printf("\n");
    return a;
}

void cfun_init() {
    (hash_defq  = oblist("#defq"))  ->_.symbol.val = cell_cfun(cfun_defq);
    (hash_div   = oblist("#div"))   ->_.symbol.val = cell_cfun(cfun_div);
    (hash_eval  = oblist("#eval"))  ->_.symbol.val = cell_cfun(cfun_eval);
    (hash_gt    = oblist("#gt"))    ->_.symbol.val = cell_cfun(cfun_gt);
    (hash_if    = oblist("#if"))    ->_.symbol.val = cell_cfun(cfun_if);
    (hash_minus = oblist("#minus")) ->_.symbol.val = cell_cfun(cfun_minus);
    (hash_not   = oblist("#not"))   ->_.symbol.val = cell_cfun(cfun_not);
    (hash_plus  = oblist("#plus"))  ->_.symbol.val = cell_cfun(cfun_plus);
    (hash_quote = oblist("#quote")) ->_.symbol.val = cell_cfun(cfun_quote);
    (hash_times = oblist("#times")) ->_.symbol.val = cell_cfun(cfun_times);

    hash_f      = oblist("#f");
    hash_t      = oblist("#t");
    hash_void   = oblist("#void");
}
