/*  TAB-P
 *
 *  module sqlite
 */
#ifdef HAVE_SQLITE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
//  connect to database
//

static cell *csqlite_connect(cell *args) {
    sqlite3 *conn = NULL;
    arg0(args); // for now, but args are database, username, password etc
    int r;

    r = sqlite3_open(SQLITE_FILE, &conn);
    if (r != SQLITE_OK) {
        cell *result = error_rts("cannot init sqlite", sqlite3_errmsg(conn));
        sqlite3_close(conn);
        return result;
    }
    return make_special_sqlite_conn(conn);
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
    char buf[100]; // TODO
    sqlite3 *conn;
    sqlite3_stmt *res;

    if (!list_pop(&args, &c)) return error_rt("missing sqlite connection");
    if (!get_sqlite_conn(c, &conn, args)) return cell_error();
    if (!list_pop(&args, &t)) return error_rt("missing table name");
    if (!peek_string(t, &table, &tlen, args)) return cell_error();
    arg0(args); // for now TODO more

    // TODO SQL escape everything
    // TODO add WHERE and so on
    snprintf(buf, sizeof(buf)-1, "SELECT * FROM '%s';", table);
    cell_unref(t);

    r = sqlite3_prepare_v2(conn, buf, -1, &res, 0);
    if (r != SQLITE_OK) {
        return error_rts("sqlite query failed", sqlite3_errmsg(conn));
    }
    for (;;) {
        r = sqlite3_step(res);
        switch (r) {
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

static cell *csqlite_version(cell *c) {
    int r;
    cell *result;
    sqlite3 *conn;
    sqlite3_stmt *res;

    if (!get_sqlite_conn(c, &conn, NIL)) return cell_error();

    r = sqlite3_prepare_v2(conn, "SELECT SQLITE_VERSION()", -1, &res, 0);
    if (r != SQLITE_OK) {
        return error_rts("sqlite query failed", sqlite3_errmsg(conn));
    }

    r = sqlite3_step(res);
    if (r != SQLITE_ROW) {
        sqlite3_finalize(res);
        return error_rts("sqlite query no data", sqlite3_errmsg(conn));
    }
    // TODO
    // int cols = sqlite3_column_count(res);
    const unsigned char *s = sqlite3_column_text(res, 0);
    int n = sqlite3_column_bytes(res, 0);
    result = cell_nastring((char *)s, n);
    sqlite3_finalize(res);
    return result;
}

static cell *csqlite_insert(cell *args) {
    return error_rt("missing sqlite connection");
    // INSERT INTO table (a, b, c) VALUES('va', 'vb', 'vc');
}

static cell *csqlite_update(cell *args) {
    return error_rt("missing sqlite connection");
    // UPDATE %s SET rst='%s' WHERE x=4;
}

////////////////////////////////////////////////////////////////
//


cell *module_sqlite() {
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("connect"),    cell_cfunN(csqlite_connect));
    assoc_set(a, cell_symbol("version"),    cell_cfun1(csqlite_version));
    assoc_set(a, cell_symbol("select"),     cell_cfunN(csqlite_select));
    assoc_set(a, cell_symbol("insert"),     cell_cfunN(csqlite_insert));
    assoc_set(a, cell_symbol("update"),     cell_cfunN(csqlite_update));

    return a;
}

#endif // HAVE_SQLILTE
