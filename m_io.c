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
#include "opt.h"

static cell *io_assoc = NIL;

#define DO_MULTILINE 0 // multiline tables and assocs
#define MAX_INDENT 40

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
    if (MAX_INDENT > 0 && indent > MAX_INDENT) indent = MAX_INDENT;

    if (!ct) {
        fprintf(out, "[]"); // NIL
        return;
    } else switch (ct->type) {

    case c_LIST:
        fprintf(out, "[ ");
        show_list(out, ct);
        fprintf(out, " ]");
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
        fprintf(out, " :: "); // TODO remove
        cell_writei(out, cell_cdr(ct), indent);
        fprintf(out, ")");
        return;

    case c_RANGE:
        if (cell_car(ct)) cell_writei(out, cell_car(ct), indent);
        fprintf(out, " .. ");
        if (cell_cdr(ct)) cell_writei(out, cell_cdr(ct), indent);
        return;

    case c_LABEL:
        cell_writei(out, cell_car(ct), indent);
        fprintf(out, ": ");
        cell_writei(out, cell_cdr(ct), indent);
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

    case c_CLOSURE:
        fprintf(out, "#closure(\n%*sargs: ", indent+2,""); // TODO debug
        cell_writei(out, ct->_.cons.car->_.cons.car, indent+4);
        fprintf(out, "\n%*sbody: ", indent+2,"");
        cell_writei(out, ct->_.cons.car->_.cons.cdr, indent);
        fprintf(out, "\n%*scont: ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr, indent);
        fprintf(out, "\n%*s) ", indent,"");
        return;

    case c_CLOSURE0:
    case c_CLOSURE0T:
        fprintf(out, "#closure0(\n%*sargs: ", indent+2,""); // TODO debug
        cell_writei(out, ct->_.cons.car, indent+4);
        fprintf(out, "\n%*sbody: ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr, indent);
        fprintf(out, "\n%*s) ", indent,"");
        return;

    case c_CFUNQ:
        fprintf(out, "#cfunQ()");
        return;
    case c_CFUN0:
        fprintf(out, "#cfun0()");
        return;
    case c_CFUN1:
        fprintf(out, "#cfun1()");
        return;
    case c_CFUN2:
        fprintf(out, "#cfun2()");
        return;
    case c_CFUN3:
        fprintf(out, "#cfun3()");
        return;
    case c_CFUNN:
        fprintf(out, "#cfunN()");
        return;

    case c_ENV:
        fprintf(out, "#env(\n%*sprev: ", indent+4,""); // TODO debug
        cell_writei(out, ct->_.cons.car->_.cons.car, indent+6);
        fprintf(out, "\n%*sprog: ", indent+4,"");
        cell_writei(out, ct->_.cons.car->_.cons.cdr, indent+6);
        fprintf(out, "\n%*sassoc: ", indent+4,"");
        cell_writei(out, ct->_.cons.cdr->_.cons.car, indent+6);
        fprintf(out, "\n%*scont: ", indent+4,"");
        cell_writei(out, ct->_.cons.cdr->_.cons.cdr, indent+6);
        fprintf(out, "\n%*s)", indent+2,"");
        return;

    case c_SPECIAL:
        fprintf(out, "#special_%s()", ct->_.special.magic);
        return;

    case c_VECTOR:
        {
            int more = 0;
            index_t n = ct->_.vector.len;
            index_t i;
            fprintf(out, "[ 0: ");
            for (i = 0; i < n; ++i) {
#if DO_MULTILINE
                fprintf(out,(more ? ",\n%*s":"\n%*s"), indent+2,"");
#else
                if (more) fprintf(out, ", ");
#endif
                more = 1;
                cell_writei(out, ct->_.vector.table[i], indent+4);
            }
#if DO_MULTILINE
            fprintf(out, "\n%*s] ", indent,"");
#else
            fprintf(out, more ? " ] ":"] ");
#endif
        }
        return;

    case c_ASSOC:
        if (opt_assocsorted) {
            struct assoc_s **v = assoc2vector(ct);
            index_t n = 0;
            fprintf(out, "{ ");
            for (n = 0; v[n]; ++n) {
                fprintf(out,(n > 0 ? ",\n%*s":"\n%*s"), indent+2,"");
                cell_writei(out, v[n]->key, indent);
                fprintf(out, " : ");
                cell_writei(out, v[n]->val, indent);
	    }
            fprintf(out, "\n%*s} ", indent,"");
			free(v);
		} else {
	    struct assoc_s *p;
            int more = 0;
            index_t n = ct->_.assoc.size;
            index_t i;
			fprintf(out, "{ ");
			if (ct->_.assoc.table) for (i = 0; i < n; ++i) {
		p = ct->_.assoc.table[i];
		while (p) {
#if DO_MULTILINE
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
#if DO_MULTILINE
            fprintf(out, "\n%*s} ", indent,"");
#else
            fprintf(out, more ? " } ":"} ");
#endif
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
    return cell_void();
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
    return cell_void();
}

static cell *cfio_println(cell *args) {
    cell *v = cfio_print(args);
    fprintf(stdout, "\n");
    return v;
}

static cell *cfio_read() {
    // TODO how to deal with error messages
    // TODO threading
    // TODO should probably continue previous lxfile
    lxfile infile;
    lxfile_init(&infile, stdin);
    return expression(&infile);
}

static cell *cfio_getline() {
    ssize_t len = 0;
    char *line = lex_getline(stdin, &len);
    if (!line || len < 0) {
        return cell_void();
    }
    return cell_astring(line, len);
}

cell *module_io() {
    if (!io_assoc) {
        io_assoc = cell_assoc();

        // TODO these functions are impure
        // TODO file open/write/delete and slurp to read an entire file?
        assoc_set(io_assoc, cell_symbol("print"),   cell_cfunN(cfio_print)); // scheme 'display'
        assoc_set(io_assoc, cell_symbol("println"), cell_cfunN(cfio_println));
        assoc_set(io_assoc, cell_symbol("write"),   cell_cfunN(cfio_write));
        assoc_set(io_assoc, cell_symbol("read"),    cell_cfun0(cfio_read));
        assoc_set(io_assoc, cell_symbol("getline"), cell_cfun0(cfio_getline));
    }
    // TODO static io_assoc owns one, hard to avoid
    return cell_ref(io_assoc);
}

