/*  TAB-P
 *
 */

cell *newnode(celltype t);

void freenode(cell *node);

void node_init(); // called once at startup
void node_exit(); // called once at exit

