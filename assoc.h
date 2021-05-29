/*  TAB-P
 */

#include "type.h"

// assoc iterator
struct assoc_i {
    struct cell_s *anode;
    struct cell_s *p;
    index_t h;
    struct cell_s **sorted; // sorted vector, or NULL
};

int assoc_set(struct cell_s *anode, struct cell_s *key, struct cell_s *val);

int assoc_get(struct cell_s *anode, struct cell_s *key, struct cell_s **valuep);

void assoc_iter(struct assoc_i *ip, struct cell_s *anode);
void assoc_iter_sorted(struct assoc_i *ip, struct cell_s *anode);
struct cell_s *assoc_next(struct assoc_i *ip);

void assoc_drop(struct cell_s *anode);

