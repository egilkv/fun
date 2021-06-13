/*  TAB-P
 *
 *  module gtk
 *  https://developer.gnome.org/gtk3/stable/
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
static const char *magic_gtk_widget = "gtk_widget";
//static const char *magic_gtk_text_buf = "gtk_text_buffer";

// a is nevre unreffed
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

static int get_gtkwidget(cell *arg, GtkWidget **gpp, cell *dump) {
    return get_special(magic_gtk_widget, arg, (void **)gpp, dump);
}

////////////////////////////////////////////////////////////////
//
//  application
//  https://developer.gnome.org/gtk3/stable/GtkApplication.htm
//

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

static cell *cgtk_application_window_new(cell *app) {
    GtkWidget *window;
    GtkApplication *gp;
    if (!get_gtkapp(app, &gp, NIL)) {
        return cell_void(); // error
    }
    window = gtk_application_window_new(gp);
    // TODO error check

    return cell_special(magic_gtk_widget, (void *)window);
}

////////////////////////////////////////////////////////////////
//
//  box
//

static cell *cgtk_button_box_new(cell *flags) {
    GtkWidget *button_box;
    // TODO ignore flags
    // GTK_ORIENTATION_HORIZONTAL
    // GTK_ORIENTATION_VERTICAL
    cell_unref(flags);
    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    return cell_special(magic_gtk_widget, (void *)button_box);
}

////////////////////////////////////////////////////////////////
//
//  button
//  https://developer.gnome.org/gtk3/stable/GtkButton.html
//

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
    return cell_special(magic_gtk_widget, (void *)button);
}

// TODO gtk_button_new_with_mnemonic() see gtk_button_set_use_underline()
// gtk_button_new_from_icon_name (), use gtk_button_new() and gtk_button_set_image().instead
// TODO gtk_button_clicked (button)
// TODO gtk_button_set_relief(button, reliefstyle)
// TODO gtk_button_get_relief(button)
// TODO gtk_button_get_label(button)
// TODO gtk_button_set_label(button, label)
// TODO gtk_button_set_use_underline(button, bool)
// TODO gtk_button_get_use_underline(button) -> bool
// TODO gtk_button_set_focus_on_click(button, bool)
// TODO gtk_button_get_focus_on_click(button) -> bool
// TODO gtk_button_set_alignment(button, xalign, yalign)
// TODO gtk_button_get_alignment(button) -> xalign, yalign
// TODO gtk_button_set_image(button, image)
// TODO gtk_button_get_image(button)
// TODO gtk_button_set_image_position(
// TODO gtk_button_get_image_position(
// TODO gtk_button_set_always_show_image ()
// TODO gtk_button_get_always_show_image ()
// TODO gtk_button_get_event_window ()

////////////////////////////////////////////////////////////////
//
//  container
//  https://developer.gnome.org/gtk3/stable/GtkContainer.html
//

static cell *cgtk_container_add(cell *args) {
    cell *widget = NIL;
    cell *add = NIL;
    GtkWidget *wp;
    GtkWidget *ap;
    if (!list_pop(&args, &widget)
     || !list_pop(&args, &add)) {
        cell_unref(widget);
        return error_rt0("needs arguments container and add");
    }
    arg0(args);
    if (!get_gtkwidget(widget, &wp, add)
     || !get_gtkwidget(add, &ap, NIL)) {
        return cell_void(); // error
    }
    gtk_container_add(GTK_CONTAINER(wp), ap);
    return cell_void();
}

static cell *cgtk_container_remove(cell *widget, cell *rem) {
    GtkWidget *wp;
    GtkWidget *ap;
    if (!get_gtkwidget(widget, &wp, rem)
     || !get_gtkwidget(rem, &ap, NIL)) {
        return cell_void(); // error
    }
    gtk_container_remove(GTK_CONTAINER(wp), ap);
    return cell_void();
}

static cell *cgtk_container_check_resize(cell *widget) {
     GtkWidget *wp;
     if (!get_gtkwidget(widget, &wp, NIL)) {
        return cell_void(); // error
     }
     gtk_container_check_resize(GTK_CONTAINER(wp));
     return cell_void();
}

// TODO gtk_container_foreach(cell *widget, ... callback) {
// TODO gtk_container_get_children(cell *widget) {
// TODO gtk_container_get_path_for_child
// TODO gtk_container_get_focus_child(cell *widget) returns widget
// TODO gtk_container_set_focus_child(cell *widget, cell *child)
// TODO gtk_container_get_focus_vadjustment(cell *widget) returns adjustment or NULL
// TODO gtk_container_set_focus_vadjustment(cell *widget, cell *adjustment)  GtkAdjustment
// TODO gtk_container_get_focus_hadjustment(cell *widget) returns adjustment or NULL
// TODO gtk_container_set_focus_hadjustment(cell *widget, cell *adjustment)  GtkAdjustment
// TODO gtk_container_child_type(cell *widget) {
// TODO gtk_container_child_get(cell *widget, property names ... ) { see also ..._with_properties
// TODO gtk_container_child_set(cell *widget, property names ... ) { see also ..._with_properties
// TODO gtk_container_child_get_property(cell *widget, cell *child, cell *propertyname) {
// TODO gtk_container_child_set_property(cell *widget, cell *child, cell *propertyname, cell *value) !
// TODO gtk_container_child_get_valist(cell *widget, cell *child, cell *valist ) {
// TODO gtk_container_child_set_valist(cell *widget, cell *child, cell *valist) {
// TODO gtk_container_child_notify(cell *widget, cell *child, cell *propertyname) {
// TODO gtk_container_child_notify_by_pspec(cell *widget, cell *child, ... ) {
// TODO gtk_container_forall(cell *widget, cell *callback ... ) {
// TODO gtk_container_propagate_draw(cell *widget
// TODO gtk_container_class_find_child_property(
// TODO gtk_container_class_install_child_property(
// TODO gtk_container_class_install_child_properties(
// TODO gtk_container_class_list_child_properties
// TODO gtk_container_class_handle_border_width

static cell *cgtk_container_get_border_width(cell *widget) {
    GtkWidget *wp;
    guint width;
    if (!get_gtkwidget(widget, &wp, NIL)) {
         return cell_void(); // error
    }
    width = gtk_container_get_border_width(GTK_CONTAINER(wp));
    return cell_integer(width);
}

static cell *cgtk_container_set_border_width(cell *widget, cell *wid) {
    GtkWidget *wp;
    integer_t width = 0;
    if (!get_gtkwidget(widget, &wp, wid)
     || !get_integer(wid, &width, NIL)) {
        return cell_void(); // error
    }
    gtk_container_set_border_width(GTK_CONTAINER(wp), width);
    return cell_void();
}

////////////////////////////////////////////////////////////////
//
//  grid
//  https://developer.gnome.org/gtk3/stable/GtkGrid.html
//

static cell *cgtk_grid_new(cell *args) {
    GtkWidget *grid;
    arg0(args);
    grid = gtk_grid_new();
    // TODO errors?
    return cell_special(magic_gtk_widget, (void *)grid);
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
    if (!get_gtkwidget(grid, &gp, widget)) {
	cell_unref(coord);
        return cell_void(); // error
    }
    if (!get_gtkwidget(widget, &wp, coord)) {
        return cell_void(); // error
    }

    for (n=0; n<4; ++n) {
	cell *v = ref_index(coord, n);
        if (!get_integer(v, &co[n], coord)) return cell_void(); // error
    }
    gtk_grid_attach(GTK_GRID(gp), wp, co[0], co[1], co[2], co[3]);
    return cell_void();
}

// TODO gtk_grid_attach_next_to(grid, child, sibling ...
// TODO gtk_grid_get_child_at(grid,
// TODO gtk_grid_insert_row(grid, position
// TODO gtk_grid_insert_column(grid, position
// TODO gtk_grid_remove_row(grid, position
// TODO gtk_grid_remove_column(grid, position
// TODO gtk_grid_insert_next_to ()
// TODO gtk_grid_set_row_homogeneous ()
// TODO gtk_grid_get_row_homogeneous ()
//      gtk_grid_set_row_spacing ()
//      gtk_grid_get_row_spacing ()
// TODO gtk_grid_set_column_homogeneous ()
// TODO gtk_grid_get_column_homogeneous ()
//      gtk_grid_set_column_spacing ()
//      gtk_grid_get_column_spacing ()
//      gtk_grid_set_baseline_row ()
//      gtk_grid_get_baseline_row ()
//      gtk_grid_set_row_baseline_position ()
//      gtk_grid_get_row_baseline_position ()

////////////////////////////////////////////////////////////////
//
//  text view
//

static cell *cgtk_text_view_new(cell *args) {
//  cell *a;
    GtkWidget *text_view;
//  GtkTextBuffer *text_buf;
    arg0(args);
#if 0
    if (list_pop(&args, &a)) {
        if (!get_text_buf_s(widget, &wp, width)) {
#endif
    text_view = gtk_text_view_new();

    return cell_special(magic_gtk_widget, (void *)text_view);
}

////////////////////////////////////////////////////////////////
//
//
//

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

////////////////////////////////////////////////////////////////
//
//
//

// TODO this may be a separate thread?
static void do_callback(GtkApplication* gp, gpointer data) {
 // assert(cell_is_func((cell *)data));
 // assert(cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_app)
 //     || cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_widget));
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
     && app->_.special.magic != magic_gtk_widget) {
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

////////////////////////////////////////////////////////////////
//
//
//

static cell *cgtk_widget_destroy(cell *widget) {
    GtkWidget *wp;
    cell_ref(widget); // extra ref
    if (!get_gtkwidget(widget, &wp, widget)) {
        return cell_void(); // error
    }
    gtk_widget_destroy(wp);
    // properly invalidate widget
    widget->_.special.ptr = NULL;
    widget->_.special.magic = NULL;
    cell_unref(widget);
    return cell_void();
}

static cell *cgtk_widget_show_all(cell *widget) {
    GtkWidget *wp;
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_void(); // error
    gtk_widget_show_all(wp);
    return cell_void();
}

////////////////////////////////////////////////////////////////
//
//  window
//  https://developer.gnome.org/gtk3/stable/GtkWindow.html
//

static cell *cgtk_window_set_title(cell *widget, cell *title) {
    GtkWidget *wp;
    char *title_s;
    if (!get_gtkwidget(widget, &wp, title)
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
    if (!get_gtkwidget(widget, &wp, width)) {
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

////////////////////////////////////////////////////////////////
//

cell *module_gtk() {
    // TODO consider cache
    cell *a = cell_assoc();

    // TODO these functions are impure
    assoc_set(a, cell_symbol("application_new"),        cell_cfunN(cgtk_application_new));
    assoc_set(a, cell_symbol("application_run"),        cell_cfun2(cgtk_application_run));
    assoc_set(a, cell_symbol("application_window_new"), cell_cfun1(cgtk_application_window_new));
    assoc_set(a, cell_symbol("button_new"),             cell_cfunN(cgtk_button_new)); // also "button_new_with_label"
    assoc_set(a, cell_symbol("button_box_new"),         cell_cfunN(cgtk_button_box_new));
    assoc_set(a, cell_symbol("container_add"),          cell_cfunN(cgtk_container_add));
    assoc_set(a, cell_symbol("container_remove"),       cell_cfun2(cgtk_container_remove));
    assoc_set(a, cell_symbol("container_check_resize"), cell_cfunN(cgtk_container_check_resize));
    assoc_set(a, cell_symbol("container_get_border_width"), cell_cfun1(cgtk_container_get_border_width));
    assoc_set(a, cell_symbol("container_set_border_width"), cell_cfun2(cgtk_container_set_border_width));
    assoc_set(a, cell_symbol("grid_new"),               cell_cfunN(cgtk_grid_new));
    assoc_set(a, cell_symbol("grid_attach"),            cell_cfunN(cgtk_grid_attach));
    assoc_set(a, cell_symbol("init"),                   cell_cfun1(cgtk_init));
    assoc_set(a, cell_symbol("main"),                   cell_cfunN(cgtk_main));
    assoc_set(a, cell_symbol("print"),                  cell_cfunN(cgtk_print));
    assoc_set(a, cell_symbol("signal_connect"),         cell_cfunN(cgtk_signal_connect));
    assoc_set(a, cell_symbol("text_view_new"),          cell_cfunN(cgtk_text_view_new));
    assoc_set(a, cell_symbol("widget_destroy"),         cell_cfun1(cgtk_widget_destroy));
    assoc_set(a, cell_symbol("window_set_title"),       cell_cfun2(cgtk_window_set_title));
    assoc_set(a, cell_symbol("window_set_default_size"), cell_cfunN(cgtk_window_set_default_size));
    assoc_set(a, cell_symbol("widget_show_all"),        cell_cfun1(cgtk_widget_show_all));

    return a;
}

