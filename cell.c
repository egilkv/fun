/*  TAB-P
 *
 */

#define CELL_C

#include <stdlib.h>
#include <string.h>
#include <assert.h>                               // TODO superflous

#include "oblist.h"
#include "node.h"
#include "cmod.h" // hash_undef

cell *cell_list(cell *car, cell *cdr) {
    cell *node = newnode(c_LIST);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

cell *cell_elist(cell *car, cell *cdr) {
    cell *node = newnode(c_ELIST);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

cell *cell_func(cell *car, cell *cdr) {
    cell *node = newnode(c_FUNC);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

cell *cell_pair(cell *car, cell *cdr) {
    cell *node = newnode(c_PAIR);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

cell *cell_keyval(cell *key, cell *val) {
    cell *node = newnode(c_KEYVAL);
    node->_.cons.car = key;
    node->_.cons.cdr = val;
    return node;
}

cell *cell_keyweak(cell *key, cell *val) {
    cell *node = newnode(c_KEYVAL);
    node->_.cons.car = key;
    node->_.cons.cdr = val;
    return node;
}

cell *cell_range(cell *car, cell *cdr) {
    cell *node = newnode(c_RANGE);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

cell *cell_label(cell *car, cell *cdr) {
    cell *node = newnode(c_LABEL);
    node->_.cons.car = car;
    node->_.cons.cdr = cdr;
    return node;
}

// TODO inline
int cell_is_list(cell *cp) {
    return cp && cp->type == c_LIST;
}

// TODO inline
int cell_is_elist(cell *cp) {
    return cp && cp->type == c_ELIST;
}

int cell_is_func(cell *cp) {
    return cp && cp->type == c_FUNC;
}

int cell_is_env(cell *cp) {
    return cp && cp->type == c_ENV;
}

int cell_is_pair(cell *cp) {
    return cp && cp->type == c_PAIR;
}

int cell_is_keyval(cell *cp) {
    return cp && (cp->type == c_KEYVAL);
}

int cell_is_range(cell *cp) {
    return cp && cp->type == c_RANGE;
}

int cell_is_label(cell *cp) {
    return cp && cp->type == c_LABEL;
}

// TODO inline
int cell_is_vector(cell *cp) {
    return cp && cp->type == c_VECTOR;
}

// TODO inline
int cell_is_symbol(cell *cp) {
    return cp && cp->type == c_SYMBOL;
}

// TODO inline
int cell_is_string(cell *cp) {
    return cp && cp->type == c_STRING;
}

// TODO inline
int cell_is_assoc(cell *cp) {
    return cp && cp->type == c_ASSOC;
}

int cell_is_special(cell *cp, const char *magic) {
    return cp && cp->type == c_SPECIAL && (!magic || cp->_.special.magic == magic);
}

// TODO inline
int cell_is_number(cell *cp) {
    return cp && cp->type == c_NUMBER;
}

int cell_is_integer(cell *cp) {
    return cp && cp->type == c_NUMBER && cp->_.n.divisor == 1;
}

// TODO inline
cell *cell_car(cell *cp) {
    assert(cp && (cp->type == c_LIST 
               || cp->type == c_FUNC
               || cp->type == c_RANGE
	       || cp->type == c_LABEL
               || cp->type == c_PAIR));
    return cp->_.cons.car;
}

cell *cell_cdr(cell *cp) {
    assert(cp && (cp->type == c_LIST 
               || cp->type == c_FUNC
               || cp->type == c_RANGE
	       || cp->type == c_LABEL
               || cp->type == c_PAIR));
    return cp->_.cons.cdr;
}

cell *cell_lambda(cell *params, cell *body) {
    cell *node = newnode(c_CLOSURE0);
    node->_.cons.car = params;
    node->_.cons.cdr = body;
    return node;
}

cell *cell_closure(cell *lambda, cell *contenv) {
    cell *node = newnode(c_CLOSURE);
    node->_.cons.car = lambda;
    node->_.cons.cdr = contenv;
    return node;
}

cell *cell_env(cell *prevenv, cell *prog, cell *assoc, cell *contenv) {
    cell *node = newnode(c_ENV);
    node->_.cons.car = cell_pair(prevenv, prog);
    node->_.cons.cdr = cell_pair(assoc, contenv);
    return node;
}

// TODO inline
// get unreffed previous environment
cell *env_prev(cell *ep) {
    assert(ep && ep->type == c_ENV && ep->_.cons.car->type == c_PAIR);
    return ep->_.cons.car->_.cons.car;
}

// TODO inline
// get unreffed previous environment, i.e. continuation
cell *env_cont_env(cell *ep) {
    assert(ep && ep->type == c_ENV && ep->_.cons.cdr->type == c_PAIR);
    return ep->_.cons.cdr->_.cons.cdr;
}

// TODO inline
// get unreffed local assoc
cell *env_assoc(cell *ep) {
    assert(ep && ep->type == c_ENV && ep->_.cons.cdr->type == c_PAIR);
    return ep->_.cons.cdr->_.cons.car;
}

// TODO inline
// get unreffed program pointer
cell *env_prog(cell *ep) {
    // assert(ep && ep->type == c_ENV && ep->_.cons.car->type == c_PAIR);
    // TODO debug
    if (!(ep && ep->type == c_ENV && ep->_.cons.car->type == c_PAIR)) {
        assert(0);
    }
    return ep->_.cons.car->_.cons.cdr;
}

// TODO inline
cell **env_progp(cell *ep) {
    assert(ep && ep->type == c_ENV && ep->_.cons.car->type == c_PAIR);
    return &(ep->_.cons.car->_.cons.cdr);
}

// pop first element off list
int list_pop(cell **cpp, cell **carp) {
    if (cell_is_list(*cpp)) {
        cell *cdr = cell_ref((*cpp)->_.cons.cdr);
        *carp = cell_ref((*cpp)->_.cons.car);
        cell_unref(*cpp);
        *cpp = cdr;
        return 1;
     } else {
        *carp = NIL;
        return 0;
     }
}

void range_split(cell *cp, cell **carp, cell **cdrp) {
    assert(cell_is_range(cp));
    *carp = cell_ref(cp->_.cons.car);
    *cdrp = cell_ref(cp->_.cons.cdr);
    cell_unref(cp);
}

void label_split(cell *cp, cell **carp, cell **cdrp) {
    assert(cell_is_label(cp));
    *carp = cell_ref(cp->_.cons.car);
    if (cdrp) *cdrp = cell_ref(cp->_.cons.cdr);
    cell_unref(cp);
}

// symbol that is not malloc'd
// TODO could optimze wrt constant symbols
cell *cell_symbol(const char *symbol) {
    return cell_asymbol(strdup(symbol));
}

// symbol that is malloc'd already
cell *cell_asymbol(char *symbol) {
    return cell_ref(asymbol_find(symbol));
}

// strong that is malloc'd already
cell *cell_astring(char_t *string, index_t length) {
    cell *node = newnode(c_STRING);
    node->_.string.ptr = string;
    node->_.string.len = length;
    return node;
}

// string that is not malloc'd already
cell *cell_nastring(char_t *na_string, index_t length) {
    char_t *string = malloc(length+1);
    assert(string);
    if (length > 0) memcpy(string, na_string, length);
    string[length] = '\0'; // make C string also
    return cell_astring(string, length);
}

// C string that is not malloc'd already
cell *cell_cstring(const char *cstring) {
    index_t length;
    if (cstring) {
        length = strlen(cstring);
    } else {
        cstring = "";
        length = 0;
    }
    return cell_nastring((char_t *)cstring, length);
}

cell *cell_number(number *np) {
    cell *node = newnode(c_NUMBER);
    node->_.n = *np;
    return node;
}

cell *cell_cfunN(cell *(*fun)(cell *)) {
    cell *node = newnode(c_CFUNN);
    node->_.cfun1.def = fun;
    return node;
}

cell *cell_cfun1(cell *(*fun)(cell *)) {
    cell *node = newnode(c_CFUN1);
    node->_.cfun1.def = fun;
    return node;
}

cell *cell_cfun2(cell *(*fun)(cell *, cell *)) {
    cell *node = newnode(c_CFUN2);
    node->_.cfun2.def = fun;
    return node;
}

cell *cell_cfunN_pure(cell *(*fun)(cell *)) {
    cell *node = cell_cfunN(fun);
    node->pure = 1;
    return node;
}

cell *cell_cfun1_pure(cell *(*fun)(cell *)) {
    cell *node = cell_cfun1(fun);
    node->pure = 1;
    return node;
}

cell *cell_cfun2_pure(cell *(*fun)(cell *, cell *)) {
   cell *node = cell_cfun2(fun);
    node->pure = 1;
    return node;
}

cell *cell_vector_nil(index_t length) {
    cell *node = NIL; // result is NIL if length==0
    if (length > 0) { // if length==0 table remains NILP
        // TODO have some sanity check on vector length
        node = newnode(c_VECTOR);
        node->_.vector.len = length;
        node->_.vector.table = malloc(length * sizeof(cell *));
        assert(node->_.vector.table);
        memset(node->_.vector.table, 0, length * sizeof(cell *)); // set to NIL
    }
    return node;
}

cell *vector_resize(cell* node, index_t newlen) {
    // TODO have some sanity check on new vector length
    index_t i;
    index_t oldlen;
    if (node == NIL) { // oldlen is zero
        node = cell_vector_nil(newlen);
        for (i = 0; i < newlen; ++i) {
            node->_.vector.table[i] = cell_ref(hash_undef);
        }

    } else {
        assert(cell_is_vector(node));
        oldlen = node->_.vector.len;
        assert(oldlen > 0);
        if (oldlen > newlen) { // shrinking?
            for (i = newlen; i < oldlen; ++i) {
                cell_unref(node->_.vector.table[i]);
            }
            if (newlen == 0) {
                free(node->_.vector.table);
                node->_.vector.table = 0;
                node->_.vector.len = 0;
                cell_unref(node);
                node = NIL;
            } else {
                node->_.vector.table = realloc(node->_.vector.table, newlen * sizeof(cell *));
                node->_.vector.len = newlen;
            }

        } else if (newlen > oldlen) { // increasing
            node->_.vector.table = realloc(node->_.vector.table, newlen * sizeof(cell *));
            assert(node->_.vector.table);
            node->_.vector.len = newlen;
            for (i = oldlen; i < newlen; ++i) {
                node->_.vector.table[i] = cell_ref(hash_undef);
            }
        }
    }
    return node;
}

// consume value, but not vector
int vector_set(cell *node, index_t index, cell *value) {
    assert(cell_is_vector(node));
    if (index < 0 || index >= node->_.vector.len) {
        // out of bounds
        cell_unref(value);
        return 0;
    }
    if (node->_.vector.table[index] != hash_undef) {
        // cannot redefine
        cell_unref(value);
        return 0;
    }
    cell_unref(hash_undef);
    node->_.vector.table[index] = value;
    return 1;
}

// ref value, leave vector
int vector_get(cell *node, index_t index, cell **valuep) {
    assert(cell_is_vector(node));
    if (index < 0 || index >= node->_.vector.len) {
        // out of bounds
        *valuep = NIL;
        return 0;
    }
    *valuep = cell_ref(node->_.vector.table[index]);
    return 1;
}

cell *cell_assoc() {
    cell *node = newnode(c_ASSOC);
    // no table required when empty
    return node;
}

cell *cell_special(const char *magic, void *ptr) {
    cell *node = newnode(c_SPECIAL);
    node->_.special.ptr = ptr;
    node->_.special.magic = magic;
    return node;
}

void cell_sweep(cell *node) {
    if (!node) return;
    if (node->mark) return;

    node->mark = 1;
    switch (node->type) {
    case c_LIST:
    case c_ELIST:
    case c_FUNC:
    case c_PAIR:
    case c_KEYVAL:
    case c_RANGE:
    case c_LABEL:
    case c_CLOSURE:
    case c_CLOSURE0:
    case c_CLOSURE0T:
    case c_DOQPUSH:
    case c_DOLPUSH:
    case c_DOGPUSH:
    case c_DOCALL1:
    case c_DOCALL2:
    case c_DOCOND:
    case c_DODEFQ:
    case c_DOREFQ:
    case c_DOLAMB:
    case c_DOLABEL:
    case c_DORANGE:
    case c_DOAPPLY:
    case c_DOPOP:
    case c_DONOOP:
        cell_sweep(node->_.cons.car);
        cell_sweep(node->_.cons.cdr);
        break;
    case c_DOCALLN:
        cell_sweep(node->_.calln.cdr);
        break;
    case c_ENV: // TODO
        cell_sweep(node->_.cons.car);
        cell_sweep(node->_.cons.cdr);
        break;
    case c_VECTOR:
        if (node->_.vector.table) {
            index_t i;
            for (i = 0; i < node->_.vector.len; ++i) {
                cell_sweep(node->_.vector.table[i]);
            }
        }
        break;
    case c_ASSOC:
        if (node->_.assoc.table) {
            index_t h;
            for (h = 0; h < node->_.assoc.size; ++h) {
                cell_sweep(node->_.assoc.table[h]);
            }
        }
        break;
    case c_SYMBOL: 
        // TODO these should exist only on oblist ?
        cell_sweep(node->_.symbol.val);
        break;
    case c_STRING:
    case c_NUMBER:
    case c_CFUN1:
    case c_CFUN2:
    case c_CFUNN:
    case c_SPECIAL:
    case c_STOP:
        break;
    case c_FREE:
        assert(0); // shall not happen
    }
}

// only to be used through cell_unref()
void cell_free1(cell *node) {
    assert(node && node->ref == 0);

    switch (node->type) {
    case c_LIST:
    case c_ELIST:
    case c_FUNC:
    case c_PAIR:
    case c_KEYVAL:
    case c_RANGE:
    case c_LABEL:
    case c_CLOSURE:
    case c_CLOSURE0:
    case c_CLOSURE0T:
    case c_DOQPUSH:
    case c_DOLPUSH:
    case c_DOGPUSH:
    case c_DOCALL1:
    case c_DOCALL2:
    case c_DOCOND:
    case c_DODEFQ:
    case c_DOREFQ:
    case c_DOLAMB:
    case c_DOLABEL:
    case c_DORANGE:
    case c_DOAPPLY:
    case c_DOPOP:
    case c_DONOOP:
        cell_unref(node->_.cons.car);
        cell_unref(node->_.cons.cdr);
        break;
    case c_DOCALLN:
        cell_unref(node->_.calln.cdr);
        break;
    case c_ENV: // TODO
        cell_unref(node->_.cons.car);
        cell_unref(node->_.cons.cdr);
        break;
    case c_SYMBOL: 
        assert(oblist_teardown); // these exist only on oblist
        cell_unref(node->_.symbol.val);
        free(node->_.symbol.nam);
        break;
    case c_STRING:
        free(node->_.string.ptr);
        break;
    case c_NUMBER:
        break;
    case c_CFUN1:
    case c_CFUN2:
    case c_CFUNN:
        break;
    case c_VECTOR:
        if (node->_.vector.table) {
            index_t i;
            cell *cp;
            for (i = 0; i < node->_.vector.len; ++i) {
                cp = node->_.vector.table[i];
                node->_.vector.table[i] = NIL; // to be safe
                cell_unref(cp);
            }
        }
        break;
    case c_ASSOC:
        assoc_drop(node);
        break;
    case c_SPECIAL:
        // TODO invoke magic free function
        break;
    case c_STOP:
        break;
    case c_FREE:
        assert(0); // shall not happen
    }
    freenode(node);
}

// asym is allocated already
// return unreffed reference
cell *cell_oblist_item(char_t *asym) {
    cell *node = newnode(c_SYMBOL);
    // TODO should create error message is reffed
    if (hash_undef) {
        // define as undefined, once hash_undef is established
        node->_.symbol.val = cell_ref(hash_undef);
    }
    node->_.symbol.nam = asym; // allocated already
    return node;
}

