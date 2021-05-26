/*  TAB-P
 */

#include "type.h"

// NOTE: also update it_name in "err.c"
enum it_ {
   it_QUOTE,   // 0
   it_PLUS,
   it_MINUS,
   it_MULT,
   it_DIV,
   it_LT,      // 5
   it_NOT,
   it_GT,
   it_EQ,
   it_LTEQ,
   it_GTEQ,    // 10
   it_NTEQ,
   it_EQEQ,
   it_AMP,
   it_AND,
   it_STOP,    // 15
   it_RANGE,
   it_COMMA,
   it_COLON,
   it_SEMI,
   it_QUEST,   // 20
   it_BAR,
   it_OR,
   it_LPAR,
   it_RPAR,
   it_LBRK,    // 25
   it_RBRK,
   it_LBRC,
   it_RBRC,
   it_NUMBER,
   it_STRING,  // 30
   it_SYMBOL
} ;

typedef enum it_ token;

struct file_s {
    FILE *f;
    short is_terminal;
    short is_eof;
    int lineno;
    ssize_t index; // index of next character, starting at 0
    char *linebuf; // for terminal, allocated, NUL terminated
    ssize_t linelen;
} ;

typedef struct file_s lxfile;

struct item_s {
    token type;
    integer_t ivalue;
    integer_t divisor;
    real_t fvalue;
    real_t decimal;
    index_t slen;
    char_t *svalue;
};

typedef struct item_s item;

item *lexical(lxfile *in);
void lxfile_init(lxfile *in, FILE *f);
const char *lxfile_info(lxfile *in);

char *lex_getline(FILE *f, ssize_t *lenp);

void dropitem(item *it);
void pushitem(item *it);

