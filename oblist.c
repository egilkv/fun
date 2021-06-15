/*  TAB-P
 *
 *  TODO should all symbols have values?
 *  Or only symbols that have been defined? Probably
 */

#include <stdio.h> // TODO debug only
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "oblist.h"
#include "node.h"
#include "opt.h"

#define OBLIST_HASH_SIZE 256

struct ob_entry {
    struct ob_entry *next;
    cell *symdef;
};

int oblist_teardown = 0; // for assert() only

static struct ob_entry* ob_table[OBLIST_HASH_SIZE];

static unsigned int hash_string(const char *sym) {
    unsigned int hash = 0;
    assert(sym);
    while (*sym) {
	hash = (hash >> 10) ^ (hash << 3) ^ *sym++;
    }
    return hash % OBLIST_HASH_SIZE;
}

// add symbol to oblist, with value
// symbol is alloc'd as needed
// val is consumed
cell *symbol_set(const char *sym, cell *val) {
    cell *ob = asymbol_find(strdup(sym)); // TODO strdup not required if symbol exists already
    oblist_set(ob, val);
    return ob;
}

// add symbol to oblist, no value
// symbol is alloc'd as needed
cell *symbol_peek(const char *sym) {
    return asymbol_find(strdup(sym)); // TODO strdup not required if symbol exists already
}

// add symbol to oblist, value is itself
// symbol is alloc'd as needed
cell *symbol_self(const char *sym) {
    cell *def = symbol_set(sym, NIL); // set NIL value initially
    oblist_set(def, cell_ref(def)); // TODO consider weak binding, if possible
    return def;
}

// find or create symbol
// assume symbol is malloc()'d already
// return unreffed symbol
cell *asymbol_find(char *sym) {
    struct ob_entry **pp;
    int hash = hash_string(sym);
    pp = &(ob_table[hash]);
    while (*pp) {
        assert(cell_is_symbol((*pp)->symdef));
        if (strcmp((*pp)->symdef->_.symbol.nam, sym) == 0) {
	    // exists already
	    free(sym);
            return (*pp)->symdef;
	}
	pp = &(*pp)->next;
    }
    // not found, make entry
    *pp = malloc(sizeof(struct ob_entry));
    assert(*pp);
    (*pp)->next = 0;
    return (*pp)->symdef = cell_oblist_item(sym);
}

// define value of variable
// val is consumed
void oblist_set(cell *sym, cell *val) {
    assert(cell_is_symbol(sym));
    cell_unref(sym->_.symbol.val);
    sym->_.symbol.val = val;
}

// search oblist for text matches, return allocated match in turn
// TODO BUG: static state
char *oblist_search(const char *lookfor, int state) {
    static int h;
    static struct ob_entry *p;
    int len = strlen(lookfor);
    char *result = NULL;
    if (!state) {
	h = 0;
	p = ob_table[0];
    }
    for (;;) {
	while (p) {
	    struct ob_entry *q = p->next;
            assert(cell_is_symbol(p->symdef));
	    if (strncmp(p->symdef->_.symbol.nam, lookfor, len) == 0) {
		result = strdup(p->symdef->_.symbol.nam);
	    }
	    p = q;
	    if (result) return result;
	}
	++h;
	if (h >= OBLIST_HASH_SIZE) return NULL; // list exhausted
	p = ob_table[h];
    }
}

// do sweep and mark all cells we can reach
integer_t oblist_sweep() {
    int h;
    struct ob_entry *p;

    // mark every thing we can reach
    for (h = 0; h < OBLIST_HASH_SIZE; ++h) {
	p = ob_table[h];
	while (p) {
            assert(cell_is_symbol(p->symdef));
            // TODO check if marked already. is that OK?
            p->symdef->mark = 1;
            cell_sweep(p->symdef->_.symbol.val);
            p = p->next;
	}
    }
    // then sweep up
    return node_gc_cleanup();
}

// clean up on exit
static void oblist_exit() {
    int h;
    struct ob_entry *p;

    // do garbage collection first
    oblist_sweep();

    oblist_teardown = 1;
    if (opt_showoblist) printf("\n\noblist:\n");
    for (h = 0; h < OBLIST_HASH_SIZE; ++h) {
	p = ob_table[h];
	ob_table[h] = 0;
	while (p) {
	    struct ob_entry *q = p->next;
            cell *v;
            assert(cell_is_symbol(p->symdef));
	    if (opt_showoblist) printf("%s ", p->symdef->_.symbol.nam);
            v = p->symdef->_.symbol.val;
            p->symdef->_.symbol.val = NIL; // get rid of any circular defs
            cell_unref(v);
            cell_unref(p->symdef);
	    free(p);
	    p = q;
	}
    }
    oblist_teardown = 0;
    if (opt_showoblist) printf("\n");

    // thereafter, clean up chunks
    node_exit();
}

void oblist_init() {
    node_init();

    atexit(oblist_exit);
}
