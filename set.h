/*	TAB-P
 *
 */

struct keyival_s {
	const char *str;
	cell *key; // must be symbol
	integer_t ival;
};

typedef struct keyival_s set;

void set_init(set *sp);

int get_fromset(set *sp, cell *key, integer_t *ivalp);

cell *cell_fromset(set *sp, integer_t ival);

int get_maskset(set *sp, cell *key, integer_t *ivalp);

cell *cell_maskset(set *sp, integer_t ival);
