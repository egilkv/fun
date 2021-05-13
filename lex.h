/* TAB-P
 */

#include "type.h"

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
   it_ELIP,
   it_COMMA,
   it_COLON,
   it_SEMI,
   it_QUEST,   // 20
   it_LPAR,
   it_RPAR,
   it_LBRK,
   it_RBRK,
   it_LBRC,    // 25
   it_RBRC,
   it_INTEGER,
   it_STRING,
   it_SYMBOL
} ;

typedef enum it_ token;

struct item_s {
    token type;
    integer_t ivalue; // TODO union for efficiency?
    index_t slen;
    char_t *svalue;
};

typedef struct item_s item;

item *lexical(FILE *in);
void dropitem(item *it);
void pushitem(item *it);

