/*  TAB-P
 *
 *  module sqlite
 *
 *  https://sqlite.org/c3ref/intro.html
 *
 * replace snprintf with fancy strdup/cat thingy
 *
 * TODO:
 *  int sqlite3_busy_handler(sqlite3*,int(*)(void*,int),void*);
 */
#ifdef HAVE_SQLITE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <sqlite3.h>

#include "cmod.h"
#include "number.h"
#include "node.h" // newnode
#include "err.h"
#include "m_sqlite.h"

#define SQLITE_FILE "sqlite.sql" // TODP

// TODO
#if 0
unsigned long
mySQLITE_real_escape_string_quote(MYSQL *mysql,
                               char *to,
                                const char *from,
                                unsigned long length,
                                char quote)
#endif

static cell *bind_sqlite_conn();

////////////////////////////////////////////////////////////////
//
//  magic types
//

static const char *magic_sqlite_conn(void *ptr) {
    if (ptr) {
        // free up resource
        sqlite3_close((sqlite3 *) ptr);
    }
    return "sqlite_connection";
}

static cell *make_special_sqlite_conn(sqlite3 *conn) {
    return cell_special(magic_sqlite_conn, (void *)conn);
}

static int get_sqlite_conn(cell *arg, sqlite3 **connpp, cell *dump) {
    return get_special(magic_sqlite_conn, arg, (void **)connpp, dump);
}

////////////////////////////////////////////////////////////////
//
//  macros
//

////////////////////////////////////////////////////////////////
//
//  helper functions
//

// duplicate and catenate strings, terminated by NULL pointer
static char *strdupcat(char *str1, ...) {
    size_t strn = 0;
    char *strp = str1;
    char *result = NULL;
    va_list argp;
    va_start(argp, str1);
    while (strp != NULL) {
        int n = strlen(strp);
        result = realloc(result, strn+n+1);
        assert(result);
        if (n > 0) memcpy(result+strn, strp, n);
        result[strn+n] = '\0';
        strn += n;
        strp = va_arg(argp, char *);
    }
    return result;
}

static cell *csqlite_connect(cell *args, int mode) {
    sqlite3 *conn = NULL;
    cell *u;
    char_t *uri = NULL; // filename or possibly a URI
    index_t ulen = 0;
    int r;

    if (!list_pop(&args, &u)) return error_rt("missing database");
    if (!peek_string(u, &uri, &ulen, args)) return cell_error();
    arg0(args); // for now TODO more

    r = sqlite3_open_v2(uri, &conn, mode, NULL);
    cell_unref(u);
    if (r != SQLITE_OK) {
	cell *result = error_rts("cannot open sqlite database", sqlite3_errmsg(conn));
        sqlite3_close(conn);
        return result;
    }
    return cell_bind(make_special_sqlite_conn(conn), bind_sqlite_conn());
}

static cell *select_list(cell *c, const char *select) {
    sqlite3 *conn;
    int r;
    cell *result = NIL;
    cell **nextp = &result;
    sqlite3_stmt *res;

    if (!get_sqlite_conn(c, &conn, NIL)) return cell_error();

    r = sqlite3_prepare_v2(conn, select, -1, &res, 0);
    if (r != SQLITE_OK) {
        return error_rts("sqlite query failed", sqlite3_errmsg(conn));
    }
    for (;;) {
        switch ((r = sqlite3_step(res))) {
        case SQLITE_ROW:
            {
                const unsigned char *s = sqlite3_column_text(res, 0);
                int n = sqlite3_column_bytes(res, 0);
                *nextp = cell_list(cell_nastring((char *)s, n), NIL);
                nextp = &((*nextp)->_.cons.cdr);
            }
            break;
        default:
            sqlite3_finalize(res);
            cell_unref(result);
            return error_rts("sqlite query error", sqlite3_errmsg(conn));

        case SQLITE_DONE:
            sqlite3_finalize(res);
            return result;
        }
    }
}

static cell *select_column(cell *c, const char *select, int column) {
    cell *result = NIL;
    cell **nextp = &result;
    sqlite3 *conn;
    sqlite3_stmt *res;
    int r;
    cell *item;

    if (!get_sqlite_conn(c, &conn, NIL)) return cell_error();

    r = sqlite3_prepare_v2(conn, select, -1, &res, 0);
    if (r != SQLITE_OK) {
        return error_rts("sqlite query failed", sqlite3_errmsg(conn));
    }
    for (;;) {
        switch ((r = sqlite3_step(res))) {
        case SQLITE_ROW:
            {
                int cols;
                if ((cols = sqlite3_column_count(res)) >= column) {
                    const unsigned char *s = sqlite3_column_text(res, column); // UTF8
                    int n = sqlite3_column_bytes(res, column);
                    assert(s != NULL); // TODO better
                    item = cell_nastring((char *)s, n);
                }
                *nextp = cell_list(item, NIL);
                nextp = &((*nextp)->_.cons.cdr);
            }
            break;
        case SQLITE_BUSY:
            // TODO bug
        case SQLITE_ERROR:
        case SQLITE_MISUSE:
            sqlite3_finalize(res);
            cell_unref(result);
            return error_rts("sqlite column query error", sqlite3_errmsg(conn));

        case SQLITE_DONE:
            sqlite3_finalize(res);
            return result;
        }
    }
}

////////////////////////////////////////////////////////////////
//
//  connect to database
//

static cell *csqlite_open(cell *args) {
    return csqlite_connect(args, SQLITE_OPEN_READWRITE);
}

static cell *csqlite_create(cell *args) {
    // note: will not complain if file exists
    return csqlite_connect(args, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
}

////////////////////////////////////////////////////////////////
//
//  read data
//

static cell *csqlite_select(cell *args) {
    cell *c;
    cell *t = NIL;
    char_t *table = NULL;
    index_t tlen = 0;
    cell *result = NIL;
    cell **nextp = &result;
    int r;
    char *select;
    sqlite3 *conn;
    sqlite3_stmt *res;

    if (!list_pop(&args, &c)) return error_rt("missing sqlite connection");
    if (!get_sqlite_conn(c, &conn, args)) return cell_error();
    if (!list_pop(&args, &t)) return error_rt("missing table name");
    if (!peek_string(t, &table, &tlen, args)) return cell_error();
    arg0(args); // for now TODO more

    // TODO SQL escape everything
    // TODO add WHERE and so on
    select = strdupcat("SELECT * FROM '", table, "'", NULL);
    cell_unref(t);

    r = sqlite3_prepare_v2(conn, select, -1, &res, 0);
    free(select);
    if (r != SQLITE_OK) {
        return error_rts("sqlite query failed", sqlite3_errmsg(conn));
    }
    for (;;) {
        switch ((r = sqlite3_step(res))) {
        case SQLITE_ROW:
            {
                int i;
                cell *rowp = NIL;
                int cols;
                if ((cols = sqlite3_column_count(res)) > 0) {
                    rowp = cell_assoc();
                    for (i = 0; i < cols; ++i) {
                        cell *item = NIL;
                        const char *name = sqlite3_column_name(res, i); // UTF8

                         // TODO sqlite3_value *sqlite3_column_value(sqlite3_stmt*, int iCol);
                        switch (sqlite3_column_type(res, i)) {
                        case SQLITE_INTEGER:
                            item = cell_integer(sqlite3_column_int64(res, i));
                            break;
                        case SQLITE_FLOAT:
                            item = cell_real(sqlite3_column_double(res, i));
                            break;
                        case SQLITE_TEXT:
                            {
                                const unsigned char *s = sqlite3_column_text(res, i); // UTF8
                                int n = sqlite3_column_bytes(res, i);
                                item = cell_nastring((char *)s, n);
                            }
                            break;
                        case SQLITE_BLOB:
                            {
                                const void *blob = sqlite3_column_blob(res, i);
                                int n = sqlite3_column_bytes(res, i);
                                item = cell_nastring((char *)blob, n);
                            }
                            break;
                        case SQLITE_NULL:
                            item = NIL;
                            break;
                        }
                        assoc_set(rowp, cell_symbol(name), item);
                    }
                }
                *nextp = cell_list(rowp, NIL);
                nextp = &((*nextp)->_.cons.cdr);
            }
            break;
        case SQLITE_BUSY:
            sqlite3_finalize(res);
            cell_unref(result);
            return error_rt("sqlite busy"); // TODO not satisfactory

        case SQLITE_ERROR:
        case SQLITE_MISUSE:
            sqlite3_finalize(res);
            cell_unref(result);
            return error_rts("sqlite query error", sqlite3_errmsg(conn));

        case SQLITE_DONE:
            sqlite3_finalize(res);
            return result;
        }
    }
}

static cell *csqlite_version(cell *args) {
    int r;
    sqlite3 *conn = NULL;
    cell *result;
    arg0(args);
    r = sqlite3_open_v2(":memory:", &conn, SQLITE_OPEN_READWRITE, NULL);
    if (r != SQLITE_OK) {
        result = error_rts("sqlite open failed", sqlite3_errmsg(conn));
        sqlite3_close(conn);
    } else {
        cell *c = make_special_sqlite_conn(conn);
        result = select_list(c, "SELECT SQLITE_VERSION()");
        if (cell_is_list(result)) { // return single item
            cell *r = cell_ref(cell_car(result));
            cell_unref(result);
            result = r;
        }
        cell_unref(c);
    }
    return result;
}

static cell *csqlite_tables(cell *c) {
    return select_list(c, "SELECT name FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%' ");
}

// TODO should return a list of symbols not strings
static cell *csqlite_columns(cell *c, cell *t) {
    char_t *table = NULL;
    index_t tlen = 0;
    char *select;
    cell *result;

    if (!peek_string(t, &table, &tlen, c)) return cell_error();

    // TODO SQL escape everything
    select = strdupcat("SELECT * FROM pragma_table_info('", table, "')", NULL);
    cell_unref(t);

    // colunms are: cid|name|type|notnull|dflt_value|pk
    // type can be TEXT, INTEGER, key
    result = select_column(c, select, 1);
    free(select);
    return result;
}

// TODO if argument is a list or vector, insert those
static cell *csqlite_insert(cell *args) {
    cell *c;
    cell *t = NIL;
    cell *a = NIL;
    char_t *table = NULL;
    index_t tlen = 0;
    struct assoc_i iter;
    cell *item;
    int i;
    int r;
    cell *err;
    char *keys = NULL;
    int keys_len = 0;
    int nvals = 0;
    char *binds = NULL;
    char *select;
    sqlite3 *conn;
    sqlite3_stmt *stmt;

    // TODO arg3()
    if (!list_pop(&args, &c)) return error_rt("missing sqlite connection");
    if (!get_sqlite_conn(c, &conn, args)) return cell_error();
    if (!list_pop(&args, &t)) return error_rt("missing table name");
    if (!peek_string(t, &table, &tlen, args)) return cell_error();
    if (!list_pop(&args, &a)) {
        cell_unref(t);
        return error_rt("missing assoc table");
    }
    if (!cell_assoc(a)) {
        cell_unref(t);
        return error_rt1("not an assoc table", a);
    }
    arg0(args); // for now TODO more

    // TODO this is really inelegant
    // iterate through all keys in assoc
    assoc_iter(&iter, a);
    while ((item = assoc_next(&iter))) {
        cell *key = assoc_key(item);
        if (keys == NULL) {
            keys = strdup(key->_.symbol.nam);
            assert(keys);
            keys_len = strlen(key->_.symbol.nam);
        } else {
            int n = strlen(key->_.symbol.nam);
            keys = realloc(keys, keys_len+1+n+1);
            assert(keys);
            keys[keys_len] = ',';
            memcpy(keys+keys_len+1, key->_.symbol.nam, n+1);
            keys_len += 1+n;
        }   
        ++nvals;
    }
    binds = malloc(1+(nvals-1)*2+1);
    for (i=0; i<nvals; ++i) {
        binds[i*2] = '?';
        binds[i*2+1] = ',';
    }
    binds[1+(nvals-1)*2] = '\0';

    //http://www.adp-gmbh.ch/sqlite/bind_insert.html
    //
    // TODO sqlescape
    select = strdupcat("INSERT INTO '", table, "' (", keys, ") VALUES (", binds, ")", NULL);
    cell_unref(t);
    r = sqlite3_prepare_v2(conn, select, -1, &stmt, 0);
    free(select);
    if (r != SQLITE_OK) {
        cell_unref(a);
        return error_rts("sqlite insert failed", sqlite3_errmsg(conn));
    }
    // TODO sqlite3_bind_parameter_count(stmt) == 3

    // iterate throuch all values in assoc
    i = 1;
    assoc_iter(&iter, a);
    while ((item = assoc_next(&iter))) {
        cell *val = assoc_val(item);
        if (val == NIL) {
            r = sqlite3_bind_null(stmt, i);
        } else switch (val->type) {
        case c_NUMBER:
            switch (val->_.n.divisor) {
            case 1:
                r = sqlite3_bind_int64(stmt, i, val->_.n.dividend.ival);
                break;
            default:
                r = sqlite3_bind_double(stmt, i, (1.0 * val->_.n.dividend.ival) / val->_.n.divisor);
                break;
            case 0: r = sqlite3_bind_double(stmt, i, val->_.n.dividend.fval);
                break;
            }
            break;
        case c_STRING:
            // TODO better, but tricky, to replace SQLITE_TRANSIENT with a pointer to a destructor
            r = sqlite3_bind_text(stmt, i, val->_.string.ptr, val->_.string.len, SQLITE_TRANSIENT);
            // r = sqlite3_bind_blob(stmt, i, const void*, int n, void(*)(void*));
            break;
            // TODO wait with unwinding
        default:
            err = error_rt1("cannot store as sqlite value", cell_ref(val));
            cell_unref(t);
            cell_unref(a);
            sqlite3_finalize(stmt);
            return err;
        }
        if (r != SQLITE_OK) {
            err = error_rts("cannot bind sqlite value", sqlite3_errmsg(conn));
            cell_unref(t);
            cell_unref(a);
            sqlite3_finalize(stmt);
            return err;
        }
        ++i;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        cell_unref(a);
        return error_rts("sqlite insert step failed", sqlite3_errmsg(conn));
    }
    // sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return cell_void();
}

#if 0
static cell *csqlite_update(cell *args) {
    return error_rt("missing sqlite connection");
    // UPDATE %s SET rst='%s' WHERE x=4;
}
#endif

////////////////////////////////////////////////////////////////
//

cell *module_sqlite() {
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("open"),       cell_cfunN(csqlite_open));
    assoc_set(a, cell_symbol("create"),     cell_cfunN(csqlite_create));
    assoc_set(a, cell_symbol("version"),    cell_cfun1(csqlite_version));

    return a;
}


static cell *bind_sqlite_conn() {
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("tables"),     cell_cfun1(csqlite_tables));
    assoc_set(a, cell_symbol("columns"),    cell_cfun2(csqlite_columns)); // TODO fields
    assoc_set(a, cell_symbol("select"),     cell_cfunN(csqlite_select));
    assoc_set(a, cell_symbol("insert"),     cell_cfunN(csqlite_insert));
//  assoc_set(a, cell_symbol("update"),     cell_cfunN(csqlite_update));

    return a;
}

#endif // HAVE_SQLILTE

