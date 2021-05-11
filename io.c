/* TAB-P
 *
 * module io
 */

#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "cell.h"
#include "io.h"
#include "cfun.h"
#include "oblist.h"

static void show_list(FILE *out, cell *ct) {
    if (!ct) {
        // end of list
    } else if (cell_is_list(ct)) {
        cell_print(out, cell_car(ct)); // TODO recursion?
        if (cell_cdr(ct)) {
            if (cell_is_list(cell_cdr(ct))) {
                fprintf(out, ", ");
                show_list(out, cell_cdr(ct));
            } else {
                fprintf(out, " . "); // TODO not supported on read
                cell_print(out, ct);
            }
        }
    } else {
        assert(0);
    }
}

// does not consume cell
void cell_print(FILE *out, cell *ct) {

    if (!ct) {
        fprintf(out, "#()");
    } else switch (ct->type) {
    case c_LIST:
        fprintf(out, "#(");
        show_list(out, ct);
        fprintf(out, ")");
        break;
    case c_PAIR:
        fprintf(out, "(");
        cell_print(out, cell_car(ct));
        fprintf(out, " : ");
        cell_print(out, cell_cdr(ct));
        fprintf(out, ")");
        break;
    case c_INTEGER:
        fprintf(out, "%ld", ct->_.ivalue);
        break;
    case c_STRING:
        {
            index_t n;
            fprintf(out, "\"");
            for (n = 0; n < ct->_.string.len; ++n) {
                char_t c = ct->_.string.ptr[n];
                switch (c) {
                case '"':
                case '\\':
                    fprintf(out, "\\%c", c);
                    break;
                case '\r':
                    fprintf(out, "\\r");
                    break;
                case '\n':
                    fprintf(out, "\\n");
                    break;
                default:
                    // TODO review carefully
                    if (iscntrl(c) || c == 0x7f) {
                        // TODO only for 7 bit control chars
                        fprintf(out, "\\%03o", c);
                    } else {
                        fputc(c, out);
                    }
                    break;
                }
            }
            fprintf(out, "\"");
        }
        break;
    case c_SYMBOL:
        fprintf(out, "%s", ct->_.symbol.nam);
        break;
    case c_LAMBDA:
        fprintf(out, "#lambda"); // TODO something better
        cell_print(out, ct->_.cons.car);
        break;
    case c_CFUN:
        fprintf(out, "#cfun"); // TODO something better
        fprintf(out, "()");
        break;
    case c_VECTOR:
        {
            int more = 0;
            index_t n = ct->_.vector.len;
            index_t i;
	    fprintf(out, "[%ld]{ ", n); // TODO make JSON
            for (i = 0; i < n; ++i) {
                if (more) fprintf(out, ", ");
                more = 1;
		cell_print(out, ct->_.vector.table[i]);
            }
            fprintf(out, more ? " } ":"} ");
        }
        break;
    case c_ASSOC:
        {
	    struct assoc_s *p;
            int more = 0;
            index_t n = ct->_.assoc.size;
            index_t i;
	    fprintf(out, "[]{ "); // TODO make JSON
	    if ( ct->_.assoc.table) for (i = 0; i < n; ++i) {
		p = ct->_.assoc.table[i];
		while (p) {
                    if (more) fprintf(out, ", ");
                    more = 1;
		    cell_print(out, p->key);
		    fprintf(out, " : ");
		    cell_print(out, p->val);
		    p = p->next;
		}
	    }
            fprintf(out, more ? " } ":"} ");
        }
        break;
    }
}

static cell *cfio_write(cell *args, environment *env) {
    cell *a;
    while (list_split(args, &a, &args)) {
        a = eval(a, env);
	cell_print(stdout, a);
	cell_unref(a);
    }
    assert(args == NIL);
    return cell_ref(hash_void);
}

static cell *cfio_print(cell *args, environment *env) {
    cell *a;
    while (list_split(args, &a, &args)) {
        a = eval(a, env);
        if (a) switch (a->type) { // NIL prints as nothing
        case c_STRING:
            fwrite(a->_.string.ptr, sizeof(char_t), a->_.string.len, stdout);
            break;
        case c_INTEGER:
        case c_SYMBOL:
            cell_print(stdout, a);
            break;
        default:
            // TODO error message??
            break;
        }
        cell_unref(a);
    }
    assert(args == NIL);
    return cell_ref(hash_void);
}

static cell *cfio_println(cell *args, environment *env) {
    cell *v = cfio_print(args, env);
    fprintf(stdout, "\n");
    return v;
}



cell *module_io() {
    cell *assoc = cell_assoc();
    assoc_set(assoc, oblists("print"),   cell_cfun(cfio_print)); // scheme 'display'
    assoc_set(assoc, oblists("println"), cell_cfun(cfio_println));
    assoc_set(assoc, oblists("write"),   cell_cfun(cfio_write));
    return assoc;
}

