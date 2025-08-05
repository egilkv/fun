/*  TAB-P
 *
 *  module io
 *  non-pure functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "cell.h"
#include "m_io.h"
#include "cmod.h"
#include "number.h"
#include "parse.h"
#include "readline.h"
#include "err.h"

#define MULTILINE_VECTOR 0  // multiline vectors
#define MULTILINE_ASSOC  1  // multiline assocs
#define MAX_INDENT      40

#define SHOW_PROG        0  // show prog of closures and environments

static cell *bind_cfio_file();
static void cell_car_cdr(const char *fn, FILE *out, cell *ct, int indent);

////////////////////////////////////////////////////////////////
//
//  magic types
//

static const char *magic_file(void *ptr) {
    if (ptr) { // free up resource
        fclose((FILE *) ptr);
    }
    return "file";
}

static cell *make_special_file(FILE *file) {
    return cell_special(magic_file, (void *)file);
}

static int get_file(cell *arg, FILE **filep, cell *dump) {
    if (!get_special(magic_file, arg, (void **)filep, dump)) return 0;
    if (*filep == NULL) {
        cell_unref(error_rt("file was closed"));
        return 0;
    }
    return 1;
}

////////////////////////////////////////////////////////////////
//
//  helpers
//

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
                fprintf(out, " :: "); // TODO should not happen, not supported on read
                cell_write(out, ct);
            }
        }
    } else {
        assert(0);
    }
}

static void show_elist(FILE *out, cell *ct) {
    while (cell_is_elist(ct)) {
        cell_write(out, ct->_.cons.car);
        fprintf(out, ", ");
        ct = ct->_.cons.cdr;
    }
    // last element
    cell_write(out, ct);
}

// does not consume cell
static void cell_writei(FILE *out, cell *ct, int indent) {
    if (MAX_INDENT > 0 && indent > MAX_INDENT) indent = MAX_INDENT;

    if (!ct) {
        fprintf(out, "[]"); // NIL
        return;
    } 
    if (ct->pmark) {
        fprintf(out, " ...."); // TODO indicating pointer to self, somehow
        return;
    }
    ct->pmark = 1; // for recursion detection

    switch (ct->type) {

    case c_LIST:
        fprintf(out, "[ ");
        show_list(out, ct);
        fprintf(out, " ]");
        break;

    case c_ELIST: // debug only TODO show difference?
        fprintf(out, "[ ");
        show_elist(out, ct);
        fprintf(out, " ]");
        break;

    case c_VECTOR:
        {
            int more = 0;
            index_t n = ct->_.vector.len;
            index_t i;
            fprintf(out, "[ 0: ");
            for (i = 0; i < n; ++i) {
#if MULTILINE_VECTOR
                fprintf(out,(more ? ",\n%*s":"\n%*s"), indent+2,"");
#else
                if (more) fprintf(out, ", ");
#endif
                more = 1;
                cell_writei(out, ct->_.vector.table[i], indent+4);
            }
#if MULTILINE_VECTOR
            fprintf(out, "\n%*s] ", indent,"");
#else
            fprintf(out, more ? " ] ":"] ");
#endif
        }
        break;

    case c_ASSOC:
        {
            struct assoc_i iter;
            cell *p;
            int more = 0;
            assoc_iter_maybe(&iter, ct);
            fprintf(out, "{ ");
            while ((p = assoc_next(&iter))) {
#if MULTILINE_ASSOC
                fprintf(out,(more ? ",\n%*s":"\n%*s"), indent+2,"");
#else
                if (more) fprintf(out, ", ");
#endif
                more = 1;
                cell_writei(out, p, indent);
	    }
#if MULTILINE_ASSOC
            fprintf(out, "\n%*s} ", indent,"");
#else
            fprintf(out, more ? " } ":"} ");
#endif
        }
        break;

    case c_KEYVAL: // only in assocs
        cell_writei(out, assoc_key(ct), indent);
        fprintf(out, ": ");
        cell_writei(out, assoc_val(ct), indent);
        break;

    case c_FUNC:
        cell_writei(out, cell_car(ct), indent);
        fprintf(out, "(");
        show_list(out, cell_cdr(ct));
        fprintf(out, ")");
        break;

    case c_PAIR:
        fprintf(out, "(");
        cell_writei(out, cell_car(ct), indent);
        fprintf(out, " :: "); // TODO remove
        cell_writei(out, cell_cdr(ct), indent);
        fprintf(out, ")");
        break;

    case c_BIND:
        fprintf(out, "#bind(");
        cell_writei(out, ct->_.cons.car, indent);
        fprintf(out, ", "); // TODO remove
        cell_writei(out, ct->_.cons.cdr, indent);
        fprintf(out, ")");
        break;

#if 1 // runtime
    case c_DOQPUSH:
        cell_car_cdr("#doqpush", out, ct, indent);
        break;

    case c_DOLPUSH:
        cell_car_cdr("#dolpush", out, ct, indent);
        break;

    case c_DOGPUSH:
        cell_car_cdr("#dogpush", out, ct, indent);
        break;

    case c_DOCALL1:
        cell_car_cdr( "#docall1", out, ct, indent);
        break;

    case c_DOCALL2:
        cell_car_cdr( "#docall2", out, ct, indent);
        break;

    case c_DOCALLN:
        fprintf(out, "#docall%lld() -> ", ct->_.calln.narg);
        cell_writei(out, ct->_.calln.cdr, indent);
        break;

    case c_DOCOND:
        fprintf(out, "#docond\n%*s#t -> ", indent+2,"");
        cell_writei(out, ct->_.cons.car, indent+4);
        fprintf(out, "\n%*s#f -> ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr, indent+4);
        break;

    case c_DOIF:
        fprintf(out, "#doif\n%*s#t -> ", indent+2,"");
        cell_writei(out, ct->_.cons.car, indent+4);
        fprintf(out, "\n%*selse -> ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr, indent+4);
        break;

    case c_DOELSE:
        fprintf(out, "#doelse\n%*s#f -> ", indent+2,"");
        cell_writei(out, ct->_.cons.car, indent+4);
        fprintf(out, "\n%*s-> ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr, indent+4);
        break;

    case c_DODEFQ:
        cell_car_cdr("#dodefq", out, ct, indent);
        break;

    case c_DOREFQ:
        cell_car_cdr("#dorefq", out, ct, indent);
        break;

    case c_DOLAMB:
        cell_car_cdr("#dolamb", out, ct, indent);
        break;

    case c_DOLABEL:
        cell_car_cdr("#dolabel", out, ct, indent);
        break;

    case c_DORANGE:
        fprintf(out, "#dorange(");
        fprintf(out, ") -> ");
        cell_writei(out, ct->_.cons.cdr, indent);
        break;

    case c_DOAPPLY:
        fprintf(out, "#doapply(");
        fprintf(out, ") -> ");
        cell_writei(out, ct->_.cons.cdr, indent);
        break;

    case c_DOPOP:
        fprintf(out, "#dopop() -> ");
        cell_writei(out, ct->_.cons.cdr, indent);
        break;

    case c_DONOOP:
        fprintf(out, "#donoop() -> ");
        cell_writei(out, ct->_.cons.cdr, indent);
        break;

    case c_SEMI:
        fprintf(out, "#semi() ");
        break;
#endif

    case c_RANGE:
        if (cell_car(ct)) cell_writei(out, cell_car(ct), indent);
        fprintf(out, " .. ");
        if (cell_cdr(ct)) cell_writei(out, cell_cdr(ct), indent);
        break;

    case c_LABEL:
        cell_writei(out, cell_car(ct), indent);
        fprintf(out, ": ");
        cell_writei(out, cell_cdr(ct), indent);
        break;

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

    case c_CLOSURE:
        fprintf(out, "#closure(\n%*sargs: ", indent+2,""); // TODO debug
        cell_writei(out, ct->_.cons.car->_.cons.car, indent+4);
#if SHOW_PROG
        fprintf(out, "\n%*sprog: ", indent+2,"");
        cell_writei(out, ct->_.cons.car->_.cons.cdr, indent);
#endif
        fprintf(out, "\n%*scont: ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr, indent);
        fprintf(out, "\n%*s)", indent,"");
        break;

    case c_CLOSURE0:
    case c_CLOSURE0T:
        fprintf(out, "#closure0(\n%*sargs: ", indent+2,""); // TODO debug
        cell_writei(out, ct->_.cons.car, indent+4);
#if SHOW_PROG
        fprintf(out, "\n%*sprog: ", indent+2,"");
        cell_writei(out, ct->_.cons.cdr, indent);
#endif
        fprintf(out, "\n%*s)", indent,"");
        break;

    case c_CFUN1:
    case c_CFUN2:
    case c_CFUNN:
    case c_CFUNR:
        fprintf(out, "#cfun()");
        break;

    case c_ENV:
        fprintf(out, "#env(\n%*sprev: ", indent+4,""); // TODO debug
        cell_writei(out, ct->_.cons.car->_.cons.car, indent+6);
#if SHOW_PROG
        fprintf(out, "\n%*sprog: ", indent+4,"");
        cell_writei(out, ct->_.cons.car->_.cons.cdr, indent+6);
#endif
        fprintf(out, "\n%*sassoc: ", indent+4,"");
        cell_writei(out, ct->_.cons.cdr->_.cons.car, indent+6);
        fprintf(out, "\n%*scont: ", indent+4,"");
        cell_writei(out, ct->_.cons.cdr->_.cons.cdr, indent+6);
        fprintf(out, "\n%*s)", indent+2,"");
        break;

    case c_CHANNEL:
        fprintf(out, "#channel(%s%s)", ct->_.channel.receivers ? "R":"", ct->_.channel.senders ? "S":""); // TODO R/W is debug
        break;

    case c_RCHANNEL:
        fprintf(out, "#rchannel(%s)", ct->_.rchannel.receivers ? "R":""); // TODO R/W is debug
        break;

    case c_SCHANNEL:
        fprintf(out, "#schannel()");
        break;

    case c_SPECIAL:
        fprintf(out, "#special(%s)", (*(ct->_.special.magicf))(NULL));
        break;

    case c_STOP:
        fprintf(out, "#stop"); // only during garbage collection cleanup
        break;

    case c_FREE: // shall not happen
        fprintf(out, "#free");
        break;
    }
    ct->pmark = 0;
}

static void cell_car_cdr(const char *fn, FILE *out, cell *ct, int indent) {
    fprintf(out, "%s(", fn);
    cell_writei(out, ct->_.cons.car, indent);
    fprintf(out, ") -> ");
    cell_writei(out, ct->_.cons.cdr, indent);
}

// does not consume cell
void cell_write(FILE *out, cell *ct) {
    cell_writei(out, ct, 0);
}

static void cell_print(FILE *out, cell *args) {
    cell *a;
    while (list_pop(&args, &a)) {
        if (a) switch (a->type) { // NIL prints as nothing
        case c_STRING:
            fwrite(a->_.string.ptr, sizeof(char_t), a->_.string.len, out);
            break;
        case c_NUMBER:
        case c_SYMBOL:
        case c_LIST: // TODO sure?
        case c_VECTOR: // TODO sure?
        case c_ASSOC: // TODO sure?
            cell_write(out, a);
            break;
        default:
            // TODO error message??
            fprintf(out, "?");
            break;
        }
        cell_unref(a);
    }
    assert(args == NIL);
}

// mode = "r" "r+" "w" "w+" "a" "a+"
static cell *file_open(cell *n, const char *mode) {
    FILE *file;
    char_t *name = NULL; // filename
    index_t nlen = 0;

    if (!peek_string(n, &name, &nlen, NIL)) return cell_error();

    file = fopen(name, mode);
    if (!file) {
        int e = errno;
        // TODO should also include filename
        cell *result = error_rts("cannot open file", strerror(e));
        cell_unref(n);
        return result;
    }
    cell_unref(n);
    return cell_bind(make_special_file(file), bind_cfio_file());
}

////////////////////////////////////////////////////////////////
//
//  functions
//

static cell *cfio_open(cell *a) {
    return file_open(a, "r+");
}

static cell *cfio_file_close(cell *f) {
    FILE *file = NULL;
    if (!peek_special(magic_file, f, (void **)&file, f)) {
        return cell_error();
    }
    if (file != NULL) { // not closed already?
        // TODO check error status
        fclose(file);
        // set magic pointer to NULL to avoid further usage
        assert(cell_is_special(f, magic_file));
        f->_.special.ptr = NULL;
    }
    cell_unref(f);
    return cell_void();
}

static cell *cfio_file_read(cell *f) {
    FILE *file = NULL;
    lxfile infile;
    // TODO the magic think should be an lcfile instead!!!
    if (!get_file(f, &file, NIL)) return cell_error();
    lxfile_init(&infile, file, NULL);
    return expression(&infile);
}

static cell *cfio_file_getline(cell *f) {
    FILE *file = NULL;
    ssize_t len = 0;
    char *line;
    if (!get_file(f, &file, NIL)) return cell_error();
    line = line_from_file(file, &len);
    if (!line || len < 0) {
        return cell_void();
    }
    return cell_astring(line, len);
}

static cell *cfio_file_print(cell *args) {
    cell *f;
    FILE *file = NULL;
    if (!list_pop(&args, &f)) return error_rt("missing file");
    if (!get_file(f, &file, NIL)) return cell_error();
    cell_print(file, args);
    return cell_void();
}

static cell *cfio_file_println(cell *args) {
    cell *f;
    FILE *file = NULL;
    if (!list_pop(&args, &f)) return error_rt("missing file");
    if (!get_file(f, &file, NIL)) return cell_error();
    cell_print(file, args);
    fprintf(file, "\n");
    return cell_void();
}


static cell *cfio_file_write(cell *args) {
    cell *f;
    FILE *file = NULL;
    cell *a;
    if (!list_pop(&args, &f)) return error_rt("missing file");
    if (!get_file(f, &file, NIL)) return cell_error();

    // TODO error handling for write
    while (list_pop(&args, &a)) {
        cell_write(file, a);
	cell_unref(a);
        fprintf(file, "\n");
    }
    return cell_void();
}

static cell *cfio_write(cell *args) {
    cell *a;
    while (list_pop(&args, &a)) {
        cell_write(stdout, a);
	cell_unref(a);
        fprintf(stdout, "\n");
    }
    return cell_void();
}

static cell *cfio_print(cell *args) {
    cell_print(stdout, args);
    return cell_void();
}

static cell *cfio_println(cell *args) {
    cell_print(stdout, args);
    fprintf(stdout, "\n");
    return cell_void();
}

static cell *cfio_read(cell *args) {
    // TODO how to deal with error messages
    // TODO threading
    // TODO should probably continue previous lxfile
    lxfile infile;
    arg0(args);
    lxfile_init(&infile, stdin, NULL);
    return expression(&infile);
}

static cell *cfio_getline(cell *args) {
    ssize_t len = 0;
    char *line;
    arg0(args);
    line = line_from_file(stdin, &len);
    if (!line || len < 0) {
        return cell_void();
    }
    return cell_astring(line, len);
}

cell *module_io() {
    cell *a = cell_assoc();

    // these functions are impure
    // TODO introduce some monad thing
    // TODO file open/write/delete and slurp to read an entire file?
    assoc_set(a, cell_symbol("open"),    cell_cfun1(cfio_open));
    assoc_set(a, cell_symbol("print"),   cell_cfunN(cfio_print)); // scheme: 'display'
    assoc_set(a, cell_symbol("println"), cell_cfunN(cfio_println));
    assoc_set(a, cell_symbol("write"),   cell_cfunN(cfio_write));
    assoc_set(a, cell_symbol("read"),    cell_cfunN(cfio_read));
    assoc_set(a, cell_symbol("getline"), cell_cfunN(cfio_getline));

    return a;
}

static cell *bind_cfio_file() {
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("write"),   cell_cfunN(cfio_file_write));
    assoc_set(a, cell_symbol("print"),   cell_cfunN(cfio_file_print));
    assoc_set(a, cell_symbol("println"), cell_cfunN(cfio_file_println));
    assoc_set(a, cell_symbol("read"),    cell_cfun1(cfio_file_read));
    assoc_set(a, cell_symbol("getline"), cell_cfun1(cfio_file_getline));
    assoc_set(a, cell_symbol("close"),   cell_cfun1(cfio_file_close));

    return a;
}
