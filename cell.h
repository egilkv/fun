/* TAB-P
 *
 */

#ifndef CELL_H

enum cell_t {
   c_CONS,
   c_SYMBOL,
   c_STRING,
   c_INTEGER,
   c_VECTOR,
   c_ASSOC,
   c_LAMBDA,
   c_CFUN
} ;

typedef unsigned long int index_t; // TODO 64 bit
typedef long int integer_t; // TODO 64 bit

struct cell_s {
    unsigned ref     : 32; // TODO tricky 64bit
    enum cell_t type : 3;
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
            struct cell_s **table; // pointer to hash
            index_t size; // size of hash table
        } assoc;
        struct {
            char *str;
        } string;
        struct {
            struct cell_s *val; // all symbols have values
            char *nam;
        } symbol;
        struct {
            struct cell_s *(*def)(struct cell_s *, struct cell_s *);
        } cfun;
    } _;
} ;

typedef enum cell_t celltype;

#define NIL ((cell *)0)

typedef struct cell_s cell;

cell * cell_ref(cell *cp);
void cell_unref(cell *cp);

cell *cell_cons(cell *car, cell *cdr);
cell *cell_symbol(char *symbol);
cell *cell_asymbol(char *symbol);
cell *cell_astring(char *string);
cell *cell_integer(long int integer);
cell *cell_vector(index_t length);
cell *cell_cfun(struct cell_s *(*fun)(struct cell_s *, struct cell_s *));

int cell_is_cons(cell *cp);
cell *cell_car(cell *cp);
cell *cell_cdr(cell *cp);
int cell_split(cell *cp, cell **carp, cell **cdrp);

int cell_is_vector(cell *cp);
int vector_set(cell *vector, index_t index, cell *value);
int vector_get(cell *node, index_t index, cell **valuep);
void vector_resize(cell *vector, index_t newlen);

int cell_is_symbol(cell *cp);

#endif

#define CELL_H
