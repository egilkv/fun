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
            char *nam;
        } symbol;
    } _;
} ;

typedef enum cell_t celltype;

typedef struct cell_s cell;

cell *cell_cons(cell *car, cell *cdr);
cell *cell_symbol(char *symbol);
cell *cell_asymbol(char *symbol);
cell *cell_astring(char *string);
cell *cell_integer(long int integer);
void cell_drop(cell *node);
