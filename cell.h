/* TAB-P
 *
 */

#ifndef CELL_H

#include "assoc.h"
#include "eval.h"
#include "type.h"

enum cell_t {
   c_LIST,
   c_PAIR,
   c_SYMBOL,
   c_STRING,
   c_INTEGER,
   c_VECTOR,
   c_ASSOC,
   c_LAMBDA,
   c_CFUN
} ;

struct cell_s {
    unsigned ref     : 32; // TODO tricky 64bit
    enum cell_t type : 4;
    union {
        struct {
            struct cell_s *car;
            struct cell_s *cdr;
        } cons;
        integer_t ivalue;
        struct {
            struct cell_s **table; // pointer to vector
            index_t len; // length of vector
        } vector;
        struct {
	    struct assoc_s **table; // pointer to hash table
            // index_t size; // TODO not used size of hash table
        } assoc;
        struct {
            char_t *ptr; // includes a trailing '\0' for easy conversion to C
            index_t len;
        } string;
        struct {
            struct cell_s *val; // all symbols have values
            char_t *nam;
        } symbol;
        struct {
            struct cell_s *(*def)(struct cell_s *, struct env_s *);
        } cfun;
    } _;
} ;

typedef enum cell_t celltype;

#define NIL ((cell *)0)

typedef struct cell_s cell;

cell * cell_ref(cell *cp);
void cell_unref(cell *cp);

cell *cell_list(cell *car, cell *cdr);
cell *cell_pair(cell *car, cell *cdr);
cell *cell_asymbol(char_t *symbol);
cell *cell_astring(char_t *string, index_t length);
cell *cell_integer(long int integer);
cell *cell_vector(index_t length);
cell *cell_assoc();
cell *cell_cfun(struct cell_s *(*fun)(struct cell_s *, struct env_s *));

int cell_is_list(cell *cp);
int cell_is_pair(cell *cp);
cell *cell_car(cell *cp);
cell *cell_cdr(cell *cp);
int list_split(cell *cp, cell **carp, cell **cdrp);
int pair_split(cell *cp, cell **carp, cell **cdrp);

int cell_is_vector(cell *cp);
int vector_set(cell *vector, index_t index, cell *value);
int vector_get(cell *node, index_t index, cell **valuep);
void vector_resize(cell *vector, index_t newlen);

int cell_is_symbol(cell *cp);

int cell_is_string(cell *cp);

int cell_is_integer(cell *cp);

#endif

#define CELL_H
