/* TAB-P
 *
 */

#ifndef CELL_H

enum cell_t {
   c_CONS,
   c_SYMBOL,
   c_STRING,
   c_INTEGER,
   c_CFUN
} ;

struct cell_s {
    enum cell_t type; // TODO inefficient
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

typedef struct cell_s cell;

cell *cell_cons(cell *car, cell *cdr);
cell *cell_symbol(char *symbol);
cell *cell_asymbol(char *symbol);
cell *cell_astring(char *string);
cell *cell_integer(long int integer);
cell *cell_cfun(struct cell_s *(*fun)(struct cell_s *));

void cell_print(cell *ct);

void cell_drop(cell *node);

#endif

#define CELL_H
