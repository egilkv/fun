/*  TAB-P
 *
 */

#ifndef CELL_H

#include "assoc.h"
#include "type.h"

enum cell_t {
   c_LIST,      // car is first item, cdr is rest of list
   c_FUNC,      // from parse: car is function, cdr is args
   c_ENV,       // car is pair, cdr is pair
   c_CLOSURE,   // car is c_CLOSURE0, cdr is continuation env
   c_CLOSURE0,  // car is parameters, cdr is the body, NIL cont env, aka lambda
   c_CLOSURE0T, // same as c_CLOSURE0, but tracing is enabled
   c_RANGE,     // car is lower, car is upper bound; both may be NIL
   c_LABEL,     // car is label, cdr is expr
   c_PAIR,      // car is left, cdr is right part
   c_KEYVAL,    // car is key, cdr is value, for assocs
   c_ELIST,     // car is first item, cdr is rest of elist, or last
   c_FREE,      // for freelist, car is next
   c_STOP,      // for garbage collection sweep phase
   c_SYMBOL,
   c_STRING,
   c_NUMBER,
   c_VECTOR,
   c_ASSOC,
   c_SPECIAL,
   c_CFUN1,     // builtin, 1 arg
   c_CFUN2,     // builtin, 2 arg
   c_CFUNN,     // builtin, N args
   c_DOQPUSH,   // push car, cdr is next
   c_DOLPUSH,   // car is local, push, cdr is next
   c_DOGPUSH,   // car is global, push, cdr is next
   c_DOCALL1,   // car is known function or NIL for pop fun, pop 1 arg, push result
   c_DOCALL2,   // car is known function or NIL for pop fun, pop 2 args, push result
   c_DOCALLN,   // car is number of args, pop func and N args, push result
   c_DOCOND,	// pop, car if true, cdr else
   c_DODEFQ,    // car is name, pop and push value, cdr is next
   c_DOREFQ,    // car is name, pop assoc, push value, cdr is next
   c_DOLAMB,    // add closure, car is cell_lambda, cdr is next
   c_DOLABEL,   // pop expr, car is label or NIL for pop, push value, cdr is next
   c_DORANGE,   // pop lower, pop upper, push result, cdr is next
   c_DOAPPLY,   // pop fun, pop tailarg, push result, cdr is next
   c_DOPOP,     // cdr is next
   c_DONOOP     // cdr is next
} ;

struct cell_s {
    unsigned ref     : 32; // TODO will limit total # of cells; 64bit
    unsigned pure    : 1;  // for constant folding and so on
    unsigned mark    : 1;  // for garbage collect
    unsigned pmark   : 1;  // for printing TODO needed?
    enum cell_t type : 6;
    union {
        struct {
            struct cell_s *car;
            struct cell_s *cdr;
        } cons;
        number n;
        struct {
            struct cell_s **table; // pointer to vector
            index_t len; // length of vector
        } vector;
        struct {
            struct cell_s **table; // pointer to hash table
	    index_t size; // TODO not used size of hash table
        } assoc;
        struct {
            void *ptr; // pointer to special content
            const char *magic; // type of special content
        } special;
        struct {
            char_t *ptr; // includes a trailing '\0' for easy conversion to C
            index_t len;
        } string;
        struct {
            struct cell_s *val; // all symbols have values
            char_t *nam;
        } symbol;
        struct {
            struct cell_s *(*def)(struct cell_s *, struct cell_s **); // 2nd arg is envp
        } cfunq;
        struct {
            struct cell_s *(*def)(struct cell_s *);
        } cfun1;
        struct {
            struct cell_s *(*def)(struct cell_s *, struct cell_s *);
        } cfun2;
        struct {
            integer_t narg;
            struct cell_s *cdr;
        } calln;
    } _;
} ;

typedef enum cell_t celltype;

#define NIL ((cell *)0)
#define NILP ((cell **)0)

typedef struct cell_s cell;

#ifdef INLINE
    #define DEFINLINE static INLINE
#else
    #define INLINE
    #ifdef CELL_C
        #define DEFINLINE
    #endif
#endif

#ifdef DEFINLINE

extern void cell_free1(cell *node);

DEFINLINE cell *cell_ref(cell *cp) {
    if (cp) ++(cp->ref);
    return cp;
}

DEFINLINE void cell_unref(cell *node) {
    if (node && --(node->ref) == 0) cell_free1(node);
}

#else

cell *cell_ref(cell *cp);
void cell_unref(cell *cp);

#endif

cell *cell_cfunN(cell *(*fun)(cell *));
cell *cell_cfun1(cell *(*fun)(cell *));
cell *cell_cfun2(cell *(*fun)(cell *, cell *));
cell *cell_cfunN_pure(cell *(*fun)(cell *));
cell *cell_cfun1_pure(cell *(*fun)(cell *));
cell *cell_cfun2_pure(cell *(*fun)(cell *, cell *));

cell *cell_list(cell *car, cell *cdr);
cell *cell_elist(cell *car, cell *cdr);
cell *cell_func(cell *car, cell *cdr);
cell *cell_pair(cell *car, cell *cdr);
cell *cell_keyval(cell *key, cell *val);
cell *cell_keyweak(cell *key, cell *val);
cell *cell_range(cell *car, cell *cdr);
cell *cell_label(cell *car, cell *cdr);
int cell_is_list(cell *cp);
int cell_is_elist(cell *cp);
int cell_is_func(cell *cp);
int cell_is_env(cell *cp);
int cell_is_pair(cell *cp);
int cell_is_keyval(cell *cp);
int cell_is_range(cell *cp);
int cell_is_label(cell *cp);
cell *cell_car(cell *cp);
cell *cell_cdr(cell *cp);
int list_pop(cell **cp, cell **carp);
void range_split(cell *cp, cell **carp, cell **cdrp);
void label_split(cell *cp, cell **carp, cell **cdrp);

cell *cell_env(cell *prev, cell *prog, cell *assoc, cell *contenv);
cell *env_prev(cell *ep);
cell *env_cont_env(cell *ep);
cell *env_assoc(cell *ep);
cell *env_prog(cell *ep);
cell **env_progp(cell *ep);

cell *cell_lambda(cell *args, cell *body);
cell *cell_closure(cell *lambda, cell *contenv);

cell *cell_vector_nil(index_t length);
int cell_is_vector(cell *cp);
int vector_set(cell *vector, index_t index, cell *value);
int vector_get(cell *node, index_t index, cell **valuep);
cell *vector_resize(cell* node, index_t newlen);

cell *cell_symbol(const char *symbol);
cell *cell_asymbol(char_t *symbol);
int cell_is_symbol(cell *cp);
cell *cell_oblist_item(char_t *asym);

cell *cell_astring(char_t *string, index_t length);
cell *cell_nastring(char_t *na_string, index_t length);
cell *cell_cstring(const char *cstring);
int cell_is_string(cell *cp);

cell *cell_assoc();
int cell_is_assoc(cell *cp);

cell *cell_special(const char *magic, void *ptr);
int cell_is_special(cell *cp, const char *magic);

cell *cell_number(number *np);
int cell_is_number(cell *cp);
int cell_is_integer(cell *cp);

void cell_sweep(cell *node);

#endif

#define CELL_H
