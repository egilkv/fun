/*  TAB-P
 *
 *
 */

cell *compile(cell *item, cell *env0);

cell *compile_thunk_n(cell *func, int args);

cell *known_thunk_a(cell *func, cell *args);
