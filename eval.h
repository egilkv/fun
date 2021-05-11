/* TAB P
 */

struct env_s {
    struct env_s *prev;
    struct cell_s *assoc; // locals
    struct cell_s *prog;
} ;

typedef struct env_s environment;

struct cell_s *eval(struct cell_s *arg, environment *env);
