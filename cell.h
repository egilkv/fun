/* TAB-P
 *
 */

#ifndef CELL_H

enum cell_t {
   c_CONS,
   c_SYMBOL,
   c_STRING,
   c_INTEGER,
   c_LAMBDA,
   c_CFUN
} ;

struct cell_s {
    enum cell_t type : 3;
    unsigned ref     : 32; // TODO tricky
    union {
        struct {
            struct cell_s *car;
            struct cell_s *cdr;
        } cons;
        long int ivalue; // TODO overlay?
        struct {
            char *str;
        } string;
        struct {
            struct cell_s *val; // all symbols have values
            char *nam;
        } symbol;
        struct {
            struct cell_s *(*def)(struct cell_s *);
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
cell *cell_cfun(struct cell_s *(*fun)(struct cell_s *));

int cell_is_cons(cell *cp);
cell *cell_car(cell *cp);
cell *cell_cdr(cell *cp);
int cell_split(cell *cp, cell **carp, cell **cdrp);

int cell_is_symbol(cell *cp);

void cell_print(cell *ct);

#endif

#define CELL_H
