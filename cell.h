/* TAB-P
 *
 */

enum cell_t {
   c_CONS,
   c_SYMBOL,
   c_STRING,
   c_INTEGER
} ;

struct cell_s {
    enum cell_t type;
    struct cell_s *car;
    struct cell_s *cdr;

    long int ivalue; // TODO overlay?
    char *svalue;
} ;

typedef enum cell_t celltype;

typedef struct cell_s cell;

cell *cell_cons(cell *car, cell *cdr);
cell *cell_symbol(char *symbol);
cell *cell_string(char *string);
cell *cell_integer(long int integer);
void cell_drop(cell *node);
