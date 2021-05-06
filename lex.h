/* TAB-P
 */


enum it_ {
   it_QUOT,    // 0
   it_PLUS,
   it_MINS,
   it_MULT,
   it_DIVS,
   it_LT,      // 5
   it_NOT,
   it_GT,
   it_EQUL,
   it_LTEQ,
   it_GTEQ,    // 10
   it_NTEQ,
   it_EQEQ,
   it_AMP,
   it_AND,
   it_STOP,    // 15
   it_ELIP,
   it_COMA,
   it_COLO,
   it_SEMI,
   it_QEST,    // 20
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
    long int ivalue; // TODO overlay?
    char *svalue;
};

typedef struct item_s item;

item *lexical();
void dropitem(item *it);
void pushitem(item *it);

