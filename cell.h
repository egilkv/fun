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
   c_CLOSURE0,  // car is parameters, cdr is the body, nil cont env, aka lambda
   c_CLOSURE0T, // same as c_CLOSURE0, but tracing is enabled
   c_RANGE,     // car is lower, car is upper bound; both may be nil
   c_LABEL,     // car is label, car is expr
   c_PAIR,      // car is left, cdr is right part
   c_KEYVAL,    // car is key, cdr is value, for assocs
   c_KEYWEAK,   // car is key, cdr is weak binding to value
   c_ELIST,     // car is first item, cdr is rest of elist, or last
   c_FREE,      // for freelist, car is next
   c_STOP,      // for garbage collection sweep phase
   c_SYMBOL,
   c_STRING,
   c_NUMBER,
   c_VECTOR,
   c_ASSOC,
   c_SPECIAL,
   c_CFUNQ,
   c_CFUN0,
   c_CFUN1,
   c_CFUN2,
   c_CFUN3,
   c_CFUNN
} ;

struct cell_s {
    unsigned ref     : 32; // TODO will limit total # of cells; 64bit
    unsigned mark    : 1;  // for garbage collect
    unsigned pmark   : 1;  // for printing TODO needed?
    enum cell_t type : 5;
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
            struct cell_s *(*def)(void);
        } cfun0;
        struct {
            struct cell_s *(*def)(struct cell_s *);
        } cfun1;
        struct {
            struct cell_s *(*def)(struct cell_s *, struct cell_s *);
        } cfun2;
        struct {
            struct cell_s *(*def)(struct cell_s *, struct cell_s *, struct cell_s *);
        } cfun3;
    } _;
} ;

typedef enum cell_t celltype;

#define NIL ((cell *)0)
#define NILP ((cell **)0)

typedef struct cell_s cell;

cell *cell_ref(cell *cp);
void cell_unref(cell *cp);

cell *cell_cfunQ(cell *(*fun)(cell *, cell **));
cell *cell_cfunN(cell *(*fun)(cell *));
cell *cell_cfun0(cell *(*fun)(void));
cell *cell_cfun1(cell *(*fun)(cell *));
cell *cell_cfun2(cell *(*fun)(cell *, cell *));
cell *cell_cfun3(cell *(*fun)(cell *, cell *, cell *));

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
int cell_is_string(cell *cp);

cell *cell_assoc();
int cell_is_assoc(cell *cp);

cell *cell_special(const char *magic, void *ptr);
int cell_is_special(cell *cp, const char *magic);

cell *cell_number(number *np);
int cell_is_number(cell *cp);
int cell_is_integer(cell *cp);

cell *eval(cell *arg, cell **envp); // defined in eval.c

void cell_sweep(cell *node);

#endif

#define CELL_H
