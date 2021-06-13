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
#include "number.h"
#include "compile.h"
#include "run.h"
#include "err.h"

static const char *magic_gtk_app = "gtk_application";
static const char *magic_gtk_wid = "gtk_widget";

// a in always unreffed
// dump is unreffed only if error
static int peek_special(const char *magic, cell *arg, void **valuep) {
    if (!cell_is_special(arg, magic)) {
        const char *m = magic ? magic : "special";
        char *errmsg = malloc(strlen(m) + 6);
        assert(errmsg);
        strcpy(errmsg, "not a ");
        strcpy(errmsg+6, m);
        cell_unref(error_rt1(errmsg, cell_ref(arg)));
        free(errmsg);
        return 0;
    }
    *valuep = arg->_.special.ptr;
    return 1;
}

// a in always unreffed
// dump is unreffed only if error
static int get_special(const char *magic, cell *arg, void **valuep, cell *dump) {
    if (!peek_special(magic, arg, valuep)) {
        cell_unref(arg);
        cell_unref(dump);
        return 0;
    }
    cell_unref(arg);
    return 1;
}

static int get_gtkapp(cell *arg, GtkApplication **gpp, cell *dump) {
    return get_special(magic_gtk_app, arg, (void **)gpp, dump);
}

static int get_wid_s(cell *arg, GtkWidget **gpp, cell *dump) {
    return get_special(magic_gtk_wid, arg, (void **)gpp, dump);
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
    if (!get_gtkapp(app, &gp, arglist)) {
        return cell_void(); // error
    }
    // TODO convert to argc, argv
    // status =
    g_application_run(G_APPLICATION(gp), 0, NULL);
    // TODO look at status
    cell_unref(arglist);
    return cell_void();
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
        if (!peek_cstring(label, &label_s, NIL)) {
            return cell_void(); // error
        }
        button = gtk_button_new_with_label(label_s);
        cell_unref(label);
    } else {
        button = gtk_button_new();
    }
    arg0(args);
    return cell_special(magic_gtk_wid, (void *)button);
}

static cell *cgtk_container_add(cell *widget, cell *add) {
    GtkWidget *wp;
    GtkWidget *ap;
    if (!get_wid_s(widget, &wp, add)
     || !get_wid_s(add, &ap, NIL)) {
        return cell_void(); // error
    }
    gtk_container_add(GTK_CONTAINER(wp), ap);
    return cell_void();
}

static cell *cgtk_container_set_border_width(cell *widget, cell *wid) {
    GtkWidget *wp;
    integer_t width = 0;
    if (!get_wid_s(widget, &wp, wid)
     || !get_integer(wid, &width, NIL)) {
        return cell_void(); // error
    }
    gtk_container_set_border_width(GTK_CONTAINER(wp), width);
    return cell_void();
}

static cell *cgtk_grid_new(cell *args) {
    GtkWidget *grid;
    arg0(args);
    grid = gtk_grid_new();
    // TODO errors?
    return cell_special(magic_gtk_wid, (void *)grid);
}

static cell *cgtk_grid_attach(cell *args) {
    cell *grid;
    cell *widget;
    cell *coord;
    GtkWidget *gp;
    GtkWidget *wp;
    integer_t co[4] = { 0, 0, 1, 1 }; // x0, y0, xn, yn
    int n;

    if (!arg3(args, &grid, &widget, &coord)) {
        return cell_void(); // error
    }
    if (!get_wid_s(grid, &gp, widget)) {
	cell_unref(coord);
        return cell_void(); // error
    }
    if (!get_wid_s(widget, &wp, coord)) {
        return cell_void(); // error
    }

    for (n=0; n<4; ++n) {
	cell *v = ref_index(coord, n);
        if (!get_integer(v, &co[n], coord)) return cell_void(); // error
    }
    gtk_grid_attach(GTK_GRID(gp), wp, co[0], co[1], co[2], co[3]);
    return cell_void();
}

static cell *cgtk_init(cell *arglist) {
    // TODO convert to argc, argv
    // see also cgtk_application_run
    gtk_init(0, NULL);
    cell_unref(arglist);
    return cell_void();
}

static cell *cgtk_main(cell *args) {
    arg0(args);
    gtk_main();
    return cell_void();
}

static cell *cgtk_print(cell *args) {
    cell *a;
    // TODO sync with io
    while (list_pop(&args, &a)) {
        if (a) switch (a->type) { // NIL prints as nothing
        case c_STRING:
            g_print("%s", a->_.string.ptr); // print to NUL
            break;
        case c_NUMBER:
            switch (a->_.n.divisor) {
            case 1:
                g_print("%lld", a->_.n.dividend.ival); // 64bit
                break;
            case 0:
                {
                    char buf[FORMAT_REAL_LEN];
                    format_real(a->_.n.dividend.fval, buf);
                    g_print("%s", buf);
                }
                break;
            default:
                {
                    char buf[FORMAT_REAL_LEN];
                    format_real((1.0 * a->_.n.dividend.ival) / a->_.n.divisor, buf);
                    g_print("%s", buf);
                }
                break;
            }
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
    return cell_void();
}

// TODO this is most probably a separate thread
static void do_callback(GtkApplication* gp, gpointer data) {
 // assert(cell_is_func((cell *)data));
 // assert(cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_app)
 //     || cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_wid));
 // assert((GtkApplication *) (cell_car(cell_cdr((cell *)data))->_.special.ptr) == gp);
 // printf("\n***callback***\n");

    // TODO this function is in principle async
    // cell_unref(error_rt1("sorry, not implemented, ignoring", cell_ref((cell *)data))); // TODO fix

    run_async(cell_ref((cell *)data));
}

static cell *cgtk_signal_connect(cell *args) {
    cell *app;
    cell *hook;
    cell *callback;
    // TODO also works for widgets, any instance
    // TODO signal names are separated by either - or _

    //GtkApplication *gp;
    gpointer *gp;
    char_t *signal;
    if (!arg3(args, &app, &hook, &callback)) {
        return cell_void(); // error
    }
    if (!peek_special((const char *)0, app, (void **)&gp)) {
        cell_unref(callback);
        cell_unref(hook); // TODO
        cell_unref(app);
        return cell_void(); // error
    }
    if (!get_symbol(hook, &signal, callback)) {
        cell_unref(app);
        return cell_void(); // error
    }
    if (app->_.special.magic != magic_gtk_app
     && app->_.special.magic != magic_gtk_wid) {
        cell_unref(error_rt1("not a widget nor application", cell_ref(app)));
        cell_unref(app);
        return cell_void(); // error
    }

    // connect where data is the function with one argument, the app
    cell *callbackprog = compile(cell_func(callback, cell_list(app, NIL)));

    g_signal_connect(gp, signal, G_CALLBACK(do_callback), callbackprog);
    // cell_unref(callbackprog); // TODO when to unref callbackprog ???

    cell_unref(app);
    return cell_void();
}

static cell *cgtk_widget_destroy(cell *widget) {
    GtkWidget *wp;
    cell_ref(widget); // extra ref
    if (!get_wid_s(widget, &wp, widget)) {
        return cell_void(); // error
    }
    gtk_widget_destroy(wp);
    // properly invalidate widget
    widget->_.special.ptr = NULL;
    widget->_.special.magic = NULL;
    cell_unref(widget);
    return cell_void();
}

static cell *cgtk_window_set_title(cell *widget, cell *title) {
    GtkWidget *wp;
    char *title_s;
    if (!get_wid_s(widget, &wp, title)
     || !peek_cstring(title, &title_s, NIL)) return cell_void(); // error
    gtk_window_set_title(GTK_WINDOW(wp), title_s);
    cell_unref(title);
    return cell_void();
}

static cell *cgtk_window_set_default_size(cell *args) {
    cell *widget;
    cell *width;
    cell *height;
    GtkWidget *wp;
    integer_t w, h;
    // TODO also works for widgets, any instance
    // TODO signal names are separated by either - or _

    if (!arg3(args, &widget, &width, &height)) {
        return cell_void(); // error
    }
    if (!get_wid_s(widget, &wp, width)) {
        cell_unref(height);
        return cell_void(); // error
    }
    if (!get_integer(width, &w, height)
     || !get_integer(height, &h, NIL)) {
        return cell_void(); // error
    }
    gtk_window_set_default_size(GTK_WINDOW(wp), w, h);
    return cell_void();
}

static cell *cgtk_widget_show_all(cell *widget) {
    GtkWidget *wp;
    if (!get_wid_s(widget, &wp, NIL)) return cell_void(); // error
    gtk_widget_show_all(wp);
    return cell_void();
}

static cell *cgtk_application_window_new(cell *app) {
    GtkWidget *window;
    GtkApplication *gp;
    if (!get_gtkapp(app, &gp, NIL)) {
        return cell_void(); // error
    }
    window = gtk_application_window_new(gp);
    // TODO error check

    return cell_special(magic_gtk_wid, (void *)window);
}

cell *module_gtk() {
    // TODO consider cache
    cell *a = cell_assoc();

    // TODO these functions are impure
    assoc_set(a, cell_symbol("application_new"),        cell_cfunN(cgtk_application_new));
    assoc_set(a, cell_symbol("application_run"),        cell_cfun2(cgtk_application_run));
    assoc_set(a, cell_symbol("application_window_new"), cell_cfun1(cgtk_application_window_new));
    assoc_set(a, cell_symbol("button_new"),             cell_cfunN(cgtk_button_new)); // also "button_new_with_label"
    assoc_set(a, cell_symbol("button_box_new"),         cell_cfunN(cgtk_button_box_new));
    assoc_set(a, cell_symbol("container_add"),          cell_cfun2(cgtk_container_add));
    assoc_set(a, cell_symbol("container_set_border_width"), cell_cfun2(cgtk_container_set_border_width));
    assoc_set(a, cell_symbol("grid_new"),               cell_cfunN(cgtk_grid_new));
    assoc_set(a, cell_symbol("grid_attach"),            cell_cfunN(cgtk_grid_attach));
    assoc_set(a, cell_symbol("init"),                   cell_cfun1(cgtk_init));
    assoc_set(a, cell_symbol("main"),                   cell_cfunN(cgtk_main));
    assoc_set(a, cell_symbol("print"),                  cell_cfunN(cgtk_print));
    assoc_set(a, cell_symbol("signal_connect"),         cell_cfunN(cgtk_signal_connect));
    assoc_set(a, cell_symbol("widget_destroy"),         cell_cfun1(cgtk_widget_destroy));
    assoc_set(a, cell_symbol("window_set_title"),       cell_cfun2(cgtk_window_set_title));
    assoc_set(a, cell_symbol("window_set_default_size"), cell_cfunN(cgtk_window_set_default_size));
    assoc_set(a, cell_symbol("widget_show_all"),        cell_cfun1(cgtk_widget_show_all));

    return a;
}

