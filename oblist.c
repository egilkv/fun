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
cell *oblistv(const char *sym, cell *val) {
    cell *ob = oblista(strdup(sym));
    oblist_set(ob, val);
    return ob;
}

// find or create symbol
// assume symbol is malloc()'d already
// return unreffed symbol
cell *oblista(char *sym) {
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

// find symbol, or create as undef as needed
// symbol will be malloc()'d
// TODO if created, state should be "not yet defined"
// return unreffed symbol
cell *oblists(const char *sym) {

    // TODO combine with oblista() above
    struct ob_entry **pp;
    int hash = hash_string(sym);
    pp = &(ob_table[hash]);
    while (*pp) {
        assert(cell_is_symbol((*pp)->symdef));
        if (strcmp((*pp)->symdef->_.symbol.nam, sym) == 0) {
	    // symbol exists already
	    return (*pp)->symdef;
	}
	pp = &(*pp)->next;
    }
    // not found, make entry
    *pp = malloc(sizeof(struct ob_entry));
    assert(*pp);
    (*pp)->next = 0;
    return (*pp)->symdef = cell_oblist_item(strdup(sym));
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

// clean up on exit
static void oblist_exit() {
    int h;
    struct ob_entry *p;
    oblist_teardown = 1;
    if (opt_showoblist) printf("\n\noblist:\n");
    for (h = 0; h < OBLIST_HASH_SIZE; ++h) {
	p = ob_table[h];
	ob_table[h] = 0;
	while (p) {
	    struct ob_entry *q = p->next;
            assert(cell_is_symbol(p->symdef));
	    if (opt_showoblist) printf("%s ", p->symdef->_.symbol.nam);
            cell_unref(p->symdef);
	    free(p);
	    p = q;
	}
    }
    oblist_teardown = 0;
    if (opt_showoblist) printf("\n");
}

void oblist_init() {
    atexit(oblist_exit);
}
