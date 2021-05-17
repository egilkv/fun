/* TAB-P
 *
 * module io
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "cell.h"
#include "io.h"
#include "cmod.h"
#include "parse.h"

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
        fprintf(out, "%lld", ct->_.ivalue);
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
    case c_CFUNQ:
    case c_CFUN0:
    case c_CFUN1:
    case c_CFUN2:
    case c_CFUN3:
    case c_CFUNN:
        fprintf(out, "#cfun()"); // TODO something better
        break;
    case c_SPECIAL:
        fprintf(out, "#special_%s()", ct->_.special.magic);
        break;
    case c_VECTOR:
        {
            int more = 0;
            index_t n = ct->_.vector.len;
            index_t i;
            fprintf(out, "[ ");
            for (i = 0; i < n; ++i) {
                if (more) fprintf(out, ", ");
                more = 1;
		cell_print(out, ct->_.vector.table[i]);
            }
            fprintf(out, more ? " ] ":"] ");
            // TODO sprinkle some newlines here and elsewhere
        }
        break;
    case c_ASSOC:
        {
	    struct assoc_s *p;
            int more = 0;
            index_t n = ct->_.assoc.size;
            index_t i;
            fprintf(out, "{ ");
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
            // TODO sprinkle some newlines here and elsewhere
        }
        break;
    }
}

static cell *cfio_write(cell *args) {
    cell *a;
    while (list_split(args, &a, &args)) {
	cell_print(stdout, a);
	cell_unref(a);
    }
    assert(args == NIL);
    return cell_ref(hash_void);
}

static cell *cfio_print(cell *args) {
    cell *a;
    while (list_split(args, &a, &args)) {
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

static cell *cfio_println(cell *args) {
    cell *v = cfio_print(args);
    fprintf(stdout, "\n");
    return v;
}

static cell *cfio_read(cell *args) {
    arg0(args);
    // TODO how to deal with error messages
    // TODO threading
    // TODO should probably continue previous lxfile
    lxfile infile;
    lxfile_init(&infile, stdin);
    return expression(&infile);
}

static cell *cfio_getline(cell *args) {
    size_t len = 1;
    char *line = malloc(1); // if 0, getline will malloc a huge buffer
    arg0(args);
    // TODO how to deal with error messages
    // TODO threading
    if (getline(&line, &len, stdin) < 0) {
        // usually and eof but could be other type of error
        // generate error message?
        // TODO errno is error
        free(line);
        return cell_ref(hash_void);
    }
    return cell_astring(line, len-1);
}

cell *module_io() {
    static cell *io_assoc = NIL;
    if (!io_assoc) {
        io_assoc = cell_assoc();

        // TODO these functions are impure
        assoc_set(io_assoc, cell_symbol("print"), cell_cfunN(cfio_print)); // scheme 'display'
        assoc_set(io_assoc, cell_symbol("println"), cell_cfunN(cfio_println));
        assoc_set(io_assoc, cell_symbol("write"), cell_cfunN(cfio_write));
        assoc_set(io_assoc, cell_symbol("read"), cell_cfunN(cfio_read));
        assoc_set(io_assoc, cell_symbol("getline"), cell_cfunN(cfio_getline));
    }
    // TODO static io_assoc owns one, hard to avoid
    return cell_ref(io_assoc);
}

