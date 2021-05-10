/*  TAB P
 */

struct assoc_s {
    struct assoc_s *next;
    struct cell_s *key;
    struct cell_s *val;
};

int assoc_set(struct cell_s *anode, struct cell_s *key, struct cell_s *val);

int assoc_get(struct cell_s *anode, struct cell_s *key, struct cell_s **valuep);

void assoc_drop(struct cell_s *anode);
