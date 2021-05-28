/*  TAB-P
 */

#include "type.h"

struct assoc_s {
    struct assoc_s *next;
    struct cell_s *key;
    struct cell_s *val;
};

// assoc iterator
struct assoc_i {
    struct cell_s *anode;
    struct assoc_s *p;
    index_t h;
};

int assoc_set(struct cell_s *anode, struct cell_s *key, struct cell_s *val);

int assoc_get(struct cell_s *anode, struct cell_s *key, struct cell_s **valuep);

void assoc_iter(struct assoc_i *ip, struct cell_s *anode);
struct assoc_s *assoc_next(struct assoc_i *ip);

void assoc_drop(struct cell_s *anode);

// debug only
struct assoc_s **assoc2vector(struct cell_s *anode);

