/* TAB-P
 *
 * module io
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "cell.h"
#include "cfun.h"
#include "err.h"

static const char *magic_gtk_app = "gtk_application";
static const char *magic_gtk_wid = "gtk_widget";

// a in always unreffed
// dump is unreffed only if error
static int peek_special(cell *a, const char *magic, void **valuep, cell *dump) {
    if (!cell_is_special(a, magic)) {
        cell_unref(dump);
        cell_unref(error_rt1("not an appropriate argument", cell_ref(a)));
        return 0;
    }
    *valuep = a->_.special.ptr;
    return 1;
}

static int peek_app_s(cell *app, GtkApplication **gpp, cell *dump) {
    return peek_special(app, magic_gtk_app, (void **)gpp, dump);
}

static int peek_wid_s(cell *wid, GtkWidget **wdp, cell *dump) {
    return peek_special(wid, magic_gtk_wid, (void **)wdp, dump);
}

static cell *cgtk_application_new(cell *args) {
    cell *aname = NIL;
    cell *flags = NIL;
    cell *app;
    GtkApplication *gp;

    if (list_split(args, &aname, &args)) {
	// TODO pick string
	cell_unref(aname);
	if (list_split(args, &flags, &args)) {
	    // TODO implement
	    cell_unref(flags);
	}
	arg0(args);
    }
    // TODO
    gp = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);

    app = cell_special(magic_gtk_app, (void *)gp);

    return app;
}

static cell *cgtk_application_run(cell *app, cell *arglist) {
    GtkApplication *gp;
    if (peek_app_s(app, &gp, arglist)) {
        // TODO convert to argc, argv
        // status =
        g_application_run(G_APPLICATION(gp), 0, NULL);
        // TODO look at status
        cell_unref(arglist);
    }
    return app;
}

static cell *cgtk_print(cell *args) {
    cell *a;
    while (list_split(args, &a, &args)) {
        if (a) switch (a->type) { // NIL prints as nothing
        case c_STRING:
            g_print("%s", a->_.string.ptr); // print to NUL
            break;
        case c_INTEGER:
            g_print("%lld", a->_.ivalue);
            break;
        case c_SYMBOL:
            g_print("%s", a->_.symbol.nam);
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

static void do_callback(GtkApplication* gp, gpointer data) {
    assert(cell_is_list((cell *)data));
    assert(cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_app));
    assert((GtkApplication *) (cell_car(cell_cdr((cell *)data))->_.special.ptr) == gp);
 // printf("\n***callback***\n");

    // TODO this function is in principle async
    // TODO should be continuation
    eval(cell_ref((cell *)data), NULL);
}

static cell *cgtk_signal_connect(cell *app, cell *hook, cell *callback) {
    GtkApplication *gp;
    char_t *signal;
    if (!peek_app_s(app, &gp, callback)) {
        cell_unref(hook); // TODO
    } else if (get_symbol(hook, &signal, callback)) {

        // TODO should be continuation instead
        g_signal_connect(gp, signal, G_CALLBACK(do_callback),
                         cell_list(callback, cell_list(app, NIL)));
        // cell_unref(callback); // TODO when to unref callback ???
    }
    return app;
}

static cell *cgtk_window_set_title(cell *widget, cell *title) {
    GtkWidget *wp;
    char *title_s;
    if (peek_wid_s(widget, &wp, title)
     && get_cstring(title, &title_s, NIL)) {
        gtk_window_set_title(GTK_WINDOW(wp), title_s);
    }
    return widget;
}

static cell *cgtk_window_set_default_size(cell *widget, cell *width, cell *height) {
    GtkWidget *wp;
    integer_t w, h;
    if (!peek_wid_s(widget, &wp, width)) {
        cell_unref(height);
    } else if (get_integer(width, &w, height)
            && get_integer(height, &h, NIL)) {
        gtk_window_set_default_size(GTK_WINDOW(wp), w, h);
    }
    return widget;
}

static cell *cgtk_widget_show_all(cell *widget) {
    GtkWidget *wp;
    if (peek_wid_s(widget, &wp, NIL)) {
        gtk_widget_show_all(wp);
    }
    return widget;
}

static cell *cgtk_application_window_new(cell *app) {
    GtkWidget *window;
    GtkApplication *gp;
    if (!peek_app_s(app, &gp, NIL)) {
        cell_unref(app);
        return cell_ref(hash_void);
    }
    window = gtk_application_window_new(gp);
    // TODO error check

    cell_unref(app);
    return cell_special(magic_gtk_wid, (void *)window);
}

cell *module_gtk() {
    // TODO should probably be multiple copies of same entity
    cell *assoc = cell_assoc();
    // TODO these functions are unpure
    assoc_set(assoc, cell_symbol("application_new"), cell_cfunN(cgtk_application_new));
    assoc_set(assoc, cell_symbol("application_run"), cell_cfun2(cgtk_application_run));
    assoc_set(assoc, cell_symbol("application_window_new"), cell_cfun1(cgtk_application_window_new));
    assoc_set(assoc, cell_symbol("print"), cell_cfunN(cgtk_print));
    assoc_set(assoc, cell_symbol("signal_connect"), cell_cfun3(cgtk_signal_connect));
    assoc_set(assoc, cell_symbol("window_set_title"), cell_cfun2(cgtk_window_set_title));
    assoc_set(assoc, cell_symbol("window_set_default_size"), cell_cfun3(cgtk_window_set_default_size));
    assoc_set(assoc, cell_symbol("widget_show_all"), cell_cfun1(cgtk_widget_show_all));
    return assoc;
}

