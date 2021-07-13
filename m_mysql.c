/*  TAB-P
 *
 *  module sql
 */
#ifdef HAVE_MYSQL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mysql/mysql.h>
#undef list_pop

#include "cmod.h"
#include "number.h"
#include "node.h" // newnode
#include "err.h"
#include "m_mysql.h"

// TODO
#define SQL_SOCKET "/var/lib/mysql/mysql.sock"

// TODO
#if 0
unsigned long
mysql_real_escape_string_quote(MYSQL *mysql,
                               char *to,
                                const char *from,
                                unsigned long length,
                                char quote)
#endif

cell *bind_sql_conn();

////////////////////////////////////////////////////////////////
//
//  magic types
//

static const char *magic_sql_conn(void *ptr) {
    if (ptr) {
        // free up resource
        mysql_close((MYSQL *) ptr);
    }
    return "mysql_connection";
}

static cell *make_special_sql_conn(MYSQL *conn) {
    return cell_special(magic_sql_conn, (void *)conn);
}

static int get_sql_conn(cell *arg, MYSQL **connpp, cell *dump) {
    return get_special(magic_sql_conn, arg, (void **)connpp, dump);
}

////////////////////////////////////////////////////////////////
//
//  macros
//

////////////////////////////////////////////////////////////////
//
//  open connection to database
//

static cell *csql_open(cell *args) {
    MYSQL *conn;
    char *database = "database";
    char *user = "user";
    char *password = "password";
    arg0(args); // for now, but args are database, username, password etc

    conn = mysql_init(NULL);
    if (conn == NULL) {
        return error_rts("cannot init sql", mysql_error(conn));
    }
    if (mysql_real_connect(conn, NULL, user, password, database, 0, SQL_SOCKET, 0) == NULL) {
        cell *result = error_rts("cannot connect to sql", mysql_error(conn));
        mysql_close(conn);
        return result;
    }
    return cell_bind(make_special_sql_conn(conn), bind_sql_conn());
}

////////////////////////////////////////////////////////////////
//
//  read data  TODO select?
//

static cell *csql_select(cell *args) {
    cell *c;
    cell *t = NIL;
    char_t *table = NULL;
    index_t tlen = 0;
    cell *result = NIL;
    cell **nextp = &result;
    char buf[100]; // TODO
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;
    unsigned int cols;

    if (!list_pop(&args, &c)) return error_rt("missing sql connection");
    if (!get_sql_conn(c, &conn, args)) return cell_error();
    if (!list_pop(&args, &t)) return error_rt("missing table name");
    if (!peek_string(t, &table, &tlen, args)) return cell_error();
    arg0(args); // for now TODO

    // TODO SQL escape everything
    // TODO add WHERE and so on
    snprintf(buf, sizeof(buf)-1, "SELECT * FROM '%s';", table);
    cell_unref(t);

    if (mysql_query(conn, buf) != 0) {
        return error_rts("sql query failed", mysql_error(conn));
    }
    res = mysql_use_result(conn);
    fields = mysql_fetch_fields(res);

    // TODO use all rows...
    while ((row = mysql_fetch_row(res))) { // no result?
        cell *rowp = NIL;
        if ((cols = mysql_num_fields(res)) > 0) {
            int i;
            rowp = cell_assoc();
            // TODO result = cell_vector_nil(cols);
            for (i = 0; i < cols; ++i) {
                cell *item = NIL;
                // TODO use assoc name of thing
                // fields[i].name
                // https://dev.mysql.com/doc/dev/mysql-server/latest/structMYSQL__FIELD.html

                // obey type of thing
                switch (fields[i].type) {
                case MYSQL_TYPE_VAR_STRING:
                case MYSQL_TYPE_STRING:
                case MYSQL_TYPE_VARCHAR:
                case MYSQL_TYPE_ENUM: // fixed set
                    // TODO reverse escape SQL ??
                    item = cell_nastring(row[i], strlen(row[i]));
                    break;
                case MYSQL_TYPE_TINY:
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_LONG:
                case MYSQL_TYPE_LONGLONG: // TODO atoll
                case MYSQL_TYPE_INT24:
                    item = cell_integer(atol(row[i]));
                    break;
                case MYSQL_TYPE_NULL:
                    item = NIL;
                    break;
              //case MYSQL_TYPE_INVALID:
              // item = cell_void();
              // break;
                case MYSQL_TYPE_DECIMAL:
                case MYSQL_TYPE_NEWDECIMAL:
                case MYSQL_TYPE_FLOAT:
                case MYSQL_TYPE_DOUBLE:
                    // TODO wrong
                    item = cell_nastring(row[i], strlen(row[i]));
                    break;
                case MYSQL_TYPE_TIMESTAMP:
                case MYSQL_TYPE_DATE:
                case MYSQL_TYPE_TIME:
                case MYSQL_TYPE_DATETIME:
                case MYSQL_TYPE_YEAR:
                case MYSQL_TYPE_NEWDATE: // TODO Internal to MySQL. Not used in protocol
                case MYSQL_TYPE_TIMESTAMP2:
                case MYSQL_TYPE_DATETIME2: // TODO Internal to MySQL. Not used in protocol
                case MYSQL_TYPE_TIME2: // TODO Internal to MySQL. Not used in protocol
                    item = cell_nastring(row[i], strlen(row[i])); // TODO
                    break;
                case MYSQL_TYPE_TINY_BLOB:
                case MYSQL_TYPE_MEDIUM_BLOB:
                case MYSQL_TYPE_LONG_BLOB:
                case MYSQL_TYPE_BLOB:
                 // item = cell_nastring(row[i], strlen(row[i])); // TODO binary data
                 // break;

                case MYSQL_TYPE_BIT: // up to 4096 bits
             // case MYSQL_TYPE_TYPED_ARRAY: // Used for replication only.
             // case MYSQL_TYPE_BOOL: // Currently just a placeholder.
                case MYSQL_TYPE_JSON:
                case MYSQL_TYPE_SET: // TODO implement...
                case MYSQL_TYPE_GEOMETRY:
                    // TODO
                    item = cell_void();
                    break;
                }
                assoc_set(result, cell_symbol(fields[i].name), item);
            }
        }
        *nextp = cell_list(rowp, NIL);
        nextp = &((*nextp)->_.cons.cdr);
    }
    mysql_free_result(res);
    return result;
}

static cell *csql_insert(cell *args) {
    return error_rt("missing sql connection");
    // INSERT INTO table (a, b, c) VALUES('va', 'vb', 'vc');
}

static cell *csql_update(cell *args) {
    return error_rt("missing sql connection");
    // UPDATE %s SET rst='%s' WHERE x=4;
}

// TODO
//   version        SELECT VERSION():
//   tables         SHOW TABLES;

////////////////////////////////////////////////////////////////
//

cell *module_mysql() {
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("open"),       cell_cfunN(csql_open));

    return a;
}

cell *bind_sql_conn() {
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("select"),     cell_cfunN(csql_select));
    assoc_set(a, cell_symbol("insert"),     cell_cfunN(csql_insert));
    assoc_set(a, cell_symbol("update"),     cell_cfunN(csql_update));

    return a;
}

#endif // HAVE_MYSQL
