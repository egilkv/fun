/*  TAB-P
 *
 *  module gtk
 *
 *  TODO most functions return 1st argument, even on error??
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include <gtk/gtk.h>

#include "cmod.h"
#include "err.h"

static const char *magic_gtk_app = "gtk_application";
static const char *magic_gtk_wid = "gtk_widget";

// a in always unreffed
// dump is unreffed only if error
static int peek_special(cell *a, const char *magic, void **valuep, const char *errmsg, cell *dump) {
    if (!cell_is_special(a, magic)) {
        cell_unref(dump);
        cell_unref(error_rt1(errmsg, cell_ref(a)));
        return 0;
    }
    *valuep = a->_.special.ptr;
    return 1;
}

static int peek_app_s(cell *app, GtkApplication **gpp, cell *dump) {
    return peek_special(app, magic_gtk_app, (void **)gpp, "not a gtk application", dump);
}

static int peek_wid_s(cell *wid, GtkWidget **wdp, cell *dump) {
    return peek_special(wid, magic_gtk_wid, (void **)wdp, "not a gtk widget", dump);
}

static cell *cgtk_application_new(cell *args) {
    cell *aname = NIL;
    cell *flags = NIL;
    cell *app;
    GtkApplication *gp;

    if (list_pop(&args, &aname)) {
	// TODO pick string
	cell_unref(aname);
        if (list_pop(&args, &flags)) {
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

static cell *cgtk_button_box_new(cell *flags) {
    GtkWidget *button_box;
    // TODO ignore flags
    // GTK_ORIENTATION_HORIZONTAL
    // GTK_ORIENTATION_VERTICAL
    cell_unref(flags);
    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    return cell_special(magic_gtk_wid, (void *)button_box);
}

static cell *cgtk_button_new(cell *args) {
    GtkWidget *button;
    cell *label = NIL;
    if (list_pop(&args, &label)) {
        char *label_s;
        if (!get_cstring(label, &label_s, NIL)) {
            return cell_ref(hash_void); // error
        }
        button = gtk_button_new_with_label(label_s);
    } else {
        button = gtk_button_new();
    }
    arg0(args);
    return cell_special(magic_gtk_wid, (void *)button);
}

static cell *cgtk_container_add(cell *widget, cell *add) {
    GtkWidget *wp;
    GtkWidget *ap;
    if (peek_wid_s(widget, &wp, add)
     && peek_wid_s(add, &ap, NIL)) {
        gtk_container_add(GTK_CONTAINER(wp), ap);
    }
    return widget;
}

static cell *cgtk_container_set_border_width(cell *widget, cell *wid) {
    GtkWidget *wp;
    integer_t width;
    if (peek_wid_s(widget, &wp, wid)
     && get_integer(wid, &width, NIL)) {
        gtk_container_set_border_width(GTK_CONTAINER(wp), width);
    }
    return widget;
}

static cell *cgtk_grid_new() {
    GtkWidget *grid;
    grid = gtk_grid_new();
    // TODO errors?
    return cell_special(magic_gtk_wid, (void *)grid);
}

static cell *cgtk_grid_attach(cell *grid, cell *widget, cell *coord) {
    GtkWidget *gp;
    GtkWidget *wp;
    integer_t co[4] = { 0, 0, 1, 1 }; // x0, y0, xn, yn
    int n;

    if (!peek_wid_s(grid, &gp, widget)) {
	cell_unref(coord);
	return grid; // error
    }
    if (!peek_wid_s(widget, &wp, coord)) {
	return grid;
    }

    for (n=0; n<4; ++n) {
	cell *v = ref_index(coord, n);
        if (!get_integer(v, &co[n], coord)) return grid;
    }
    gtk_grid_attach(GTK_GRID(gp), wp, co[0], co[1], co[2], co[3]);
    return grid;
}

static cell *cgtk_init(cell *arglist) {
    // TODO convert to argc, argv
    // see also cgtk_application_run
    gtk_init(0, NULL);
    cell_unref(arglist);
    return cell_ref(hash_void);
}

static cell *cgtk_main() {
    gtk_main();
    return cell_ref(hash_void);
}

static cell *cgtk_print(cell *args) {
    cell *a;
    while (list_pop(&args, &a)) {
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
    assert(cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_app)
        || cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_wid));
 // assert((GtkApplication *) (cell_car(cell_cdr((cell *)data))->_.special.ptr) == gp);
 // printf("\n***callback***\n");

    // TODO this function is in principle async
    // TODO should be continuation
    eval(cell_ref((cell *)data), NULL);
}

static cell *cgtk_signal_connect(cell *app, cell *hook, cell *callback) {
    // TODO also works for widgets, any instance
    // TODO signal names are separated by either - or _

    //GtkApplication *gp;
    gpointer *gp;
    char_t *signal;
    if (!peek_special(app, (const char *)0, (void **)&gp, "not a gtk entity", callback)) {
        cell_unref(hook); // TODO
    } else if (get_symbol(hook, &signal, callback)) {
        if (app->_.special.magic != magic_gtk_app
         && app->_.special.magic != magic_gtk_wid) {
            cell_unref(error_rt1("not a ht widget or application", cell_ref(app)));
            return app;
        } else {

            // TODO should be continuation instead
            g_signal_connect(gp, signal, G_CALLBACK(do_callback),
                             cell_list(callback, cell_list(app, NIL)));
            // cell_unref(callback); // TODO when to unref callback ???
        }
    }
    return app;
}

static cell *cgtk_widget_destroy(cell *widget) {
    GtkWidget *wp;
    if (peek_wid_s(widget, &wp, NIL)) {
        gtk_widget_destroy(wp);
    }
    // TODO properly invalidate widget
    widget->_.special.ptr = NULL;
    widget->_.special.magic = NULL;
    return widget;
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
    static cell *gtk_assoc = NIL;
    if (!gtk_assoc) {
        gtk_assoc = cell_assoc();

        // TODO these functions are impure
        assoc_set(gtk_assoc, cell_symbol("application_new"), cell_cfunN(cgtk_application_new));
        assoc_set(gtk_assoc, cell_symbol("application_run"), cell_cfun2(cgtk_application_run));
        assoc_set(gtk_assoc, cell_symbol("application_window_new"), cell_cfun1(cgtk_application_window_new));
        assoc_set(gtk_assoc, cell_symbol("button_new"), cell_cfunN(cgtk_button_new)); // also "button_new_with_label"
        assoc_set(gtk_assoc, cell_symbol("button_box_new"), cell_cfunN(cgtk_button_box_new));
        assoc_set(gtk_assoc, cell_symbol("container_add"), cell_cfun2(cgtk_container_add));
        assoc_set(gtk_assoc, cell_symbol("container_set_border_width"), cell_cfun2(cgtk_container_set_border_width));
        assoc_set(gtk_assoc, cell_symbol("grid_new"), cell_cfun0(cgtk_grid_new));
        assoc_set(gtk_assoc, cell_symbol("grid_attach"), cell_cfun3(cgtk_grid_attach));
        assoc_set(gtk_assoc, cell_symbol("init"), cell_cfun1(cgtk_init));
        assoc_set(gtk_assoc, cell_symbol("main"), cell_cfun0(cgtk_main));
        assoc_set(gtk_assoc, cell_symbol("print"), cell_cfunN(cgtk_print));
        assoc_set(gtk_assoc, cell_symbol("signal_connect"), cell_cfun3(cgtk_signal_connect));
        assoc_set(gtk_assoc, cell_symbol("widget_destroy"), cell_cfun1(cgtk_widget_destroy));
        assoc_set(gtk_assoc, cell_symbol("window_set_title"), cell_cfun2(cgtk_window_set_title));
        assoc_set(gtk_assoc, cell_symbol("window_set_default_size"), cell_cfun3(cgtk_window_set_default_size));
        assoc_set(gtk_assoc, cell_symbol("widget_show_all"), cell_cfun1(cgtk_widget_show_all));
    }
    // TODO static io_assoc owns one, but hard to avoid
    return cell_ref(gtk_assoc);
}

