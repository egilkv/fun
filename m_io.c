/*  TAB-P
 *
 *  module io
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "cell.h"
#include "m_io.h"
#include "cmod.h"
#include "number.h"
#include "parse.h"

static cell *io_assoc = NIL;

static void show_list(FILE *out, cell *ct) {
    if (!ct) {
        // end of list
    } else if (cell_is_list(ct)) {
        cell_write(out, cell_car(ct)); // TODO recursion?
        if (cell_cdr(ct)) {
            if (cell_is_list(cell_cdr(ct))) {
                fprintf(out, ", ");
                show_list(out, cell_cdr(ct));
            } else {
                fprintf(out, " . "); // TODO not supported on read
                cell_write(out, ct);
            }
        }
    } else {
        assert(0);
    }
}

// does not consume cell
static void cell_writei(FILE *out, cell *ct, int indent) {

    if (!ct) {
        fprintf(out, "#()");
        return;
    } else switch (ct->type) {

    case c_LIST:
        fprintf(out, "#(");
        show_list(out, ct);
        fprintf(out, ")");
        return;

    case c_FUNC:
        cell_writei(out, cell_car(ct), indent);
        fprintf(out, "(");
        show_list(out, cell_cdr(ct));
        fprintf(out, ")");
        return;

    case c_PAIR:
        fprintf(out, "(");
        cell_writei(out, cell_car(ct), indent);
        fprintf(out, " : ");
        cell_writei(out, cell_cdr(ct), indent);
        fprintf(out, ")");
        return;

    case c_NUMBER:
        switch (ct->_.n.divisor) {
        case 1:
            fprintf(out, "%lld", ct->_.n.dividend.ival); // 64bit
            break;
        case 0:
            {
                char buf[FORMAT_REAL_LEN];
                format_real(ct->_.n.dividend.fval, buf);
                fprintf(out, "%s", buf);
            }
            break;
        default:
            fprintf(out, "%lld/%lld", ct->_.n.dividend.ival, ct->_.n.divisor); // 64bit
            break;
        }
        return;

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
        return;

    case c_SYMBOL:
        fprintf(out, "%s", ct->_.symbol.nam);
        return;

    case c_CONT:
        fprintf(out, "#cont(\n%*sargs: ", indent+2,""); // TODO debug
        cell_writei(out, ct->_.cons.car->_.cons.car, indent+4);
        fprintf(out, "\n%*sbody: ", indent+2,"");
        cell_writei(out, ct->_.cons.car->_.cons.cdr, indent);
        fprintf(out, "\n%*scont: ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr, indent);
        fprintf(out, "\n%*s) ", indent,"");
        return;

    case c_LAMBDA:
        fprintf(out, "#lambda"); // TODO something better
        cell_writei(out, ct->_.cons.car, indent);
        return;

    case c_CFUNQ:
    case c_CFUN0:
    case c_CFUN1:
    case c_CFUN2:
    case c_CFUN3:
    case c_CFUNN:
        fprintf(out, "#cfun()"); // TODO something better
        return;

    case c_ENV:
        fprintf(out, "#env(\n%*sprev: ", indent+2,""); // TODO debug
        cell_writei(out, ct->_.cons.car->_.cons.car, indent+4);
        fprintf(out, "\n%*sprog: ", indent+2,"");
        cell_writei(out, ct->_.cons.car->_.cons.cdr, indent+4);
        fprintf(out, "\n%*sassoc: ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr->_.cons.car, indent+4);
        fprintf(out, "\n%*scont: ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr->_.cons.cdr, indent+4);
        fprintf(out, "\n%*s)", indent,"");
        return;

    case c_SPECIAL:
        fprintf(out, "#special_%s()", ct->_.special.magic);
        return;

    case c_VECTOR:
        {
            int more = 0;
            index_t n = ct->_.vector.len;
            index_t i;
            fprintf(out, "[ ");
            for (i = 0; i < n; ++i) {
#if 0 // TODO multiline
                fprintf(out,(more ? ",\n%*s":"\n%*s"), indent+2,"");
#else
                if (more) fprintf(out, ", ");
#endif
                more = 1;
                cell_writei(out, ct->_.vector.table[i], indent+4);
            }
#if 0 // TODO multiline
            fprintf(out, "\n%*s] ", indent,"");
#else
            fprintf(out, more ? " ] ":"] ");
#endif
        }
        return;

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
#if 0 // TODO multiline
                    fprintf(out,(more ? ",\n%*s":"\n%*s"), indent+2,"");
#else
                    if (more) fprintf(out, ", ");
#endif
                    more = 1;
                    cell_writei(out, p->key, indent);
		    fprintf(out, " : ");
                    cell_writei(out, p->val, indent);
		    p = p->next;
		}
	    }
#if 0 // TODO multiline
            fprintf(out, "\n%*s} ", indent,"");
#else
            fprintf(out, more ? " } ":"} ");
#endif
            // TODO sprinkle some newlines here and elsewhere
        }
        return;
    }
    assert(0);
}

// does not consume cell
void cell_write(FILE *out, cell *ct) {
    cell_writei(out, ct, 0);
}

static cell *cfio_write(cell *args) {
    cell *a;
    while (list_pop(&args, &a)) {
        cell_write(stdout, a);
	cell_unref(a);
    }
    assert(args == NIL);
    return cell_ref(io_assoc); // return assoc
}

static cell *cfio_print(cell *args) {
    cell *a;
    while (list_pop(&args, &a)) {
        if (a) switch (a->type) { // NIL prints as nothing
        case c_STRING:
            fwrite(a->_.string.ptr, sizeof(char_t), a->_.string.len, stdout);
            break;
        case c_NUMBER:
        case c_SYMBOL:
            cell_write(stdout, a);
            break;
        default:
            // TODO error message??
            break;
        }
        cell_unref(a);
    }
    assert(args == NIL);
    return cell_ref(io_assoc); // return assoc
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
    ssize_t len = 0;
    char *line = lex_getline(stdin, &len);
    if (!line || len < 0) {
        return cell_ref(hash_void);
    }
    return cell_astring(line, len);
}

cell *module_io() {
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

