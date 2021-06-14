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
static const char *magic_gtk_textbuf = "gtk_text_buffer";

static int get_gtkapp(cell *arg, GtkApplication **gpp, cell *dump) {
    return get_special(magic_gtk_app, arg, (void **)gpp, dump);
}

static int get_gtkwidget(cell *arg, GtkWidget **gpp, cell *dump) {
    return get_special(magic_gtk_widget, arg, (void **)gpp, dump);
}

static int get_gtktextbuffer(cell *arg, GtkTextBuffer **gpp, cell *dump) {
    return get_special(magic_gtk_textbuf, arg, (void **)gpp, dump);
}

////////////////////////////////////////////////////////////////
//

#define WIDGET_VOID(gname, gtype) \
static cell *cgtk_##gname(cell *widget) { \
    GtkWidget *wp; \
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_error(); \
    gtk_##gname(gtype(wp)); \
    return cell_void(); \
}

#define WIDGET_GET_WIDGET(gname, gtype) \
static cell *cgtk_##gname(cell *widget) { \
    GtkWidget *wp; \
    GtkWidget *result; \
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_error(); \
    result = gtk_##gname(gtype(wp)); \
    return cell_special(magic_gtk_widget, (void *)result); \
}
// TODO above will create copies, ideally we should keep track of them

#define WIDGET_SET_WIDGET(gname, gtype) \
static cell *cgtk_##gname(cell *widget, cell *widget2) { \
    GtkWidget *wp, *w2p; \
    if (!get_gtkwidget(widget, &wp, widget2) \
     || !get_gtkwidget(widget2, &w2p, NIL)) return cell_error(); \
    gtk_##gname(gtype(wp), w2p); \
    return cell_void(); \
}

#define WIDGET_GET_INT(gname, gtype) \
static cell *cgtk_##gname(cell *widget) { \
    GtkWidget *wp; \
    guint ival;    \
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_error(); \
    ival = gtk_##gname(gtype(wp)); \
    return cell_integer(ival); \
}

#define WIDGET_SET_INT(gname, gtype) \
static cell *cgtk_##gname(cell *widget, cell *val) { \
    GtkWidget *wp; \
    integer_t ival = 0; \
    if (!get_gtkwidget(widget, &wp, val) \
     || !get_integer(val, &ival, NIL)) return cell_error(); \
    gtk_##gname(gtype(wp), ival); \
    return cell_void(); \
}

#define WIDGET_SET_2INT(gname, gtype) \
static cell *cgtk_##gname(cell *args) { \
    cell *widget; \
    cell *val1, *val2; \
    GtkWidget *wp; \
    integer_t ival1, ival2; \
    if (!arg3(args, &widget, &val1, &val2)) return cell_error(); \
    if (!get_gtkwidget(widget, &wp, val1)) { \
        cell_unref(val2); \
        return cell_error(); \
    } \
    if (!get_integer(val1, &ival1, val2) \
     || !get_integer(val2, &ival2, NIL)) return cell_error(); \
    gtk_##gname(gtype(wp), ival1, ival2); \
    return cell_void(); \
}

#define WIDGET_GET_BOOL(gname, gtype) \
static cell *cgtk_##gname(cell *widget) { \
    GtkWidget *wp; \
    gboolean bval; \
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_error(); \
    bval = gtk_##gname(gtype(wp)); \
    return cell_ref(bval ? hash_t : hash_f); \
}

#define WIDGET_SET_BOOL(gname, gtype) \
static cell *cgtk_##gname(cell *widget, cell *val) { \
    GtkWidget *wp; \
    int bval = 0; \
    if (!get_gtkwidget(widget, &wp, val) \
     || !get_boolean(val, &bval, NIL)) return cell_error(); \
    gtk_##gname(gtype(wp), bval); \
    return cell_void(); \
}

#define WIDGET_GET_FLOAT(gname, gtype) \
static cell *cgtk_##gname(cell *widget) { \
    GtkWidget *wp; \
    real_t rval;   \
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_error(); \
    rval = gtk_##gname(gtype(wp)); \
    return cell_real(rval); \
}

#define WIDGET_SET_FLOAT(gname, gtype) \
static cell *cgtk_##gname(cell *widget, cell *val) { \
    GtkWidget *wp; \
    real_t rval; \
    if (!get_gtkwidget(widget, &wp, val) \
     || !get_real(val, &rval, NIL)) return cell_error(); \
    gtk_##gname(gtype(wp), rval); \
    return cell_void(); \
}

#define WIDGET_GET_CSTRING(gname, gtype) \
static cell *cgtk_##gname(cell *widget) { \
    GtkWidget *wp; \
    const char *cstr; \
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_error(); \
    cstr = gtk_##gname(gtype(wp)); \
    return cell_astring(strdup(cstr), strlen(cstr)); \
}

#define WIDGET_SET_CSTRING(gname, gtype) \
static cell *cgtk_##gname(cell *widget, cell *str) { \
    GtkWidget *wp; \
    char_t *cstr; \
    if (!get_gtkwidget(widget, &wp, str) \
     || !peek_cstring(str, &cstr, NIL)) return cell_error(); \
    gtk_##gname(gtype(wp), cstr); \
    cell_unref(str); \
    return cell_void(); \
}

////////////////////////////////////////////////////////////////
//

#define DEFINE_CFUN1(gname) \
    assoc_set(a, cell_symbol(#gname), cell_cfun1(cgtk_##gname));
#define DEFINE_CFUN2(gname) \
    assoc_set(a, cell_symbol(#gname), cell_cfun2(cgtk_##gname));
#define DEFINE_CFUNN(gname) \
    assoc_set(a, cell_symbol(#gname), cell_cfunN(cgtk_##gname));

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
        return cell_error();
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
        return cell_error();
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
            return cell_error();
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
WIDGET_VOID(button_clicked, GTK_BUTTON)
// TODO gtk_button_set_relief(button, reliefstyle)
// TODO gtk_button_get_relief(button)
WIDGET_GET_CSTRING(button_get_label, GTK_BUTTON)
WIDGET_SET_CSTRING(button_set_label, GTK_BUTTON)
WIDGET_GET_BOOL(button_get_use_underline, GTK_BUTTON)
WIDGET_SET_BOOL(button_set_use_underline, GTK_BUTTON)
WIDGET_SET_WIDGET(button_set_image, GTK_BUTTON)
WIDGET_GET_WIDGET(button_get_image, GTK_BUTTON)
// TODO gtk_button_set_image_position(
// TODO gtk_button_get_image_position(
WIDGET_GET_BOOL(button_get_always_show_image, GTK_BUTTON)
WIDGET_SET_BOOL(button_set_always_show_image, GTK_BUTTON)
// TODO gtk_button_get_event_window

////////////////////////////////////////////////////////////////
//
//  container
//  https://developer.gnome.org/gtk3/stable/GtkContainer.html
//

WIDGET_SET_WIDGET(container_add, GTK_CONTAINER)
WIDGET_SET_WIDGET(container_remove, GTK_CONTAINER)

// TODO gtk_container_add_with_properties
// TODO gtk_container_get_resize_mode
// TODO gtk_container_set_resize_mode

WIDGET_VOID(container_check_resize, GTK_CONTAINER)

// TODO gtk_container_foreach(cell *widget, ... callback)
// TODO gtk_container_get_children(cell *widget)  --> gList
// TODO gtk_container_get_path_for_child          --> widget
WIDGET_GET_WIDGET(container_get_focus_child, GTK_CONTAINER)
WIDGET_SET_WIDGET(container_set_focus_child, GTK_CONTAINER)
// TODO gtk_container_get_focus_vadjustment(cell *widget) returns adjustment or NULL
// TODO gtk_container_set_focus_vadjustment(cell *widget, cell *adjustment)  GtkAdjustment
// TODO gtk_container_get_focus_hadjustment(cell *widget) returns adjustment or NULL
// TODO gtk_container_set_focus_hadjustment(cell *widget, cell *adjustment)  GtkAdjustment
// TODO gtk_container_child_type(cell *widget) --> GTYPE
// TODO gtk_container_child_get(cell *widget, property names ... ) { see also ..._with_properties
// TODO gtk_container_child_set(cell *widget, property names ... ) { see also ..._with_properties
// TODO gtk_container_child_get_property(cell *widget, cell *child, cell *propertyname) {
// TODO gtk_container_child_set_property(cell *widget, cell *child, cell *propertyname, cell *value) !
// TODO gtk_container_child_get_valist(cell *widget, cell *child, cell *valist ) {
// TODO gtk_container_child_set_valist(cell *widget, cell *child, cell *valist) {
// TODO gtk_container_child_notify(cell *widget, cell *child, cell *propertyname) {
// TODO gtk_container_child_notify_by_pspec(cell *widget, cell *child, ... ) {
// TODO gtk_container_forall(cell *widget, cell *callback ... ) {
WIDGET_GET_INT(container_get_border_width, GTK_CONTAINER)
WIDGET_SET_INT(container_set_border_width, GTK_CONTAINER)
// TODO gtk_container_propagate_draw(cell *widget
// TODO gtk_container_class_find_child_property(
// TODO gtk_container_class_install_child_property(
// TODO gtk_container_class_install_child_properties(
// TODO gtk_container_class_list_child_properties
// TODO gtk_container_class_handle_border_width


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
        return cell_error();
    }
    if (!get_gtkwidget(grid, &gp, widget)) {
	cell_unref(coord);
        return cell_error();
    }
    if (!get_gtkwidget(widget, &wp, coord)) {
        return cell_error();
    }

    for (n=0; n<4; ++n) {
	cell *v = ref_index(coord, n);
        if (!get_integer(v, &co[n], coord)) return cell_error();
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
//  image
//  https://developer.gnome.org/gtk3/stable/GtkImage.html
//

// TODO gtk_image_get_pixbuf
// TODO gtk_image_get_animation
// TODO gtk_image_get_icon_name
// TODO gtk_image_get_gicon_name
// TODO gtk_image_get_storage_type

static cell *cgtk_image_new_from_file(cell *str) {
    GtkWidget *image;
    char_t *str_ptr;
    index_t str_len;
    if (!peek_string(str, &str_ptr, &str_len, NIL)) return cell_error();
    image = gtk_image_new_from_file(str_ptr);
    cell_unref(str);
    return cell_special(magic_gtk_widget, (void *)image);
}

// TODO gtk_image_new_from_pixbuf
// TODO gtk_image_new_from_animation
// TODO gtk_image_new_from_icon_name
// TODO gtk_image_new_from_gicon
// TODO gtk_image_new_from_resource
// TODO gtk_image_new_from_surface

WIDGET_SET_CSTRING(image_set_from_file, GTK_IMAGE)

// TODO gtk_image_set_from_pixbuf
// TODO gtk_image_set_from_animation
// TODO gtk_image_set_from_icon_name
// TODO gtk_image_set_from_gicon

WIDGET_SET_CSTRING(image_set_from_resource, GTK_IMAGE)

// TODO gtk_image_set_from_gicon

WIDGET_VOID(image_clear, GTK_IMAGE)

static cell *cgtk_image_new(cell *args) {
    GtkWidget *image;
    arg0(args);
    image = gtk_image_new();
    return cell_special(magic_gtk_widget, (void *)image);
}

WIDGET_SET_INT(image_set_pixel_size, GTK_IMAGE)
WIDGET_GET_INT(image_get_pixel_size, GTK_IMAGE)

////////////////////////////////////////////////////////////////
//
//  label
//  https://developer.gnome.org/gtk3/stable/GtkLabel.html
//

static cell *cgtk_label_new(cell *str) {
    GtkWidget *label;
    char_t *str_ptr;
    index_t str_len;
    if (!peek_string(str, &str_ptr, &str_len, NIL)) {
        return cell_error();
    }
    label = gtk_label_new(str_ptr);
    cell_unref(str);
    return cell_special(magic_gtk_widget, (void *)label);
}

WIDGET_SET_CSTRING(label_set_text, GTK_LABEL)
// TODO    label_set_attributes
WIDGET_SET_CSTRING(label_set_markup, GTK_LABEL) // TODO g_markup_escape_text() or g_markup_printf_escaped():
WIDGET_SET_CSTRING(label_set_markup_with_mnemonic, GTK_LABEL) // TODO ditto
WIDGET_SET_CSTRING(label_set_pattern, GTK_LABEL)
// TODO    label_set_justify
WIDGET_SET_FLOAT(label_set_xalign, GTK_LABEL)
WIDGET_SET_FLOAT(label_set_yalign, GTK_LABEL)
// TODO    label_set_ellipsize
WIDGET_SET_INT(label_set_width_chars, GTK_LABEL)
WIDGET_SET_INT(label_set_max_width_chars, GTK_LABEL)
WIDGET_SET_BOOL(label_set_line_wrap, GTK_LABEL)
// TODP label_set_line_wrap_mode ()
WIDGET_SET_INT(label_set_lines, GTK_LABEL)
// TODO label_get_layout_offsets
WIDGET_GET_INT(label_get_mnemonic_keyval, GTK_LABEL)
WIDGET_GET_BOOL(label_get_selectable, GTK_LABEL)
WIDGET_GET_CSTRING(label_get_text, GTK_LABEL)
// TODO label_new_with_mnemonic
WIDGET_SET_2INT(label_select_region, GTK_LABEL)
// TODO label_set_mnemonic_widget
WIDGET_SET_BOOL(label_set_selectable, GTK_LABEL)
WIDGET_SET_CSTRING(label_set_text_with_mnemonic, GTK_LABEL)
// TODO gtk_label_get_attributes
// TODO gtk_label_get_justify
WIDGET_GET_FLOAT(label_get_xalign, GTK_LABEL)
WIDGET_GET_FLOAT(label_get_yalign, GTK_LABEL)
// TODO gtk_label_get_ellipsize
// TODO gtk_label_get_ellipsize
WIDGET_GET_INT(label_get_width_chars, GTK_LABEL)
WIDGET_GET_INT(label_get_max_width_chars, GTK_LABEL)
WIDGET_GET_CSTRING(label_get_label, GTK_LABEL)
// TODO ...
WIDGET_GET_INT(label_get_lines, GTK_LABEL)
// TODO ...
WIDGET_GET_BOOL(label_get_use_markup, GTK_LABEL)
WIDGET_GET_BOOL(label_get_use_underline, GTK_LABEL)
WIDGET_GET_BOOL(label_get_single_line_mode, GTK_LABEL)
WIDGET_GET_FLOAT(label_get_angle, GTK_LABEL)
WIDGET_SET_CSTRING(label_set_label, GTK_LABEL)
WIDGET_SET_BOOL(label_set_use_markup, GTK_LABEL)
WIDGET_SET_BOOL(label_set_use_underline, GTK_LABEL)
WIDGET_SET_BOOL(label_set_single_line_mode, GTK_LABEL)
WIDGET_SET_FLOAT(label_set_angle, GTK_LABEL)
WIDGET_GET_CSTRING(label_get_current_uri, GTK_LABEL)
WIDGET_SET_BOOL(label_set_track_visited_links, GTK_LABEL)
WIDGET_GET_BOOL(label_get_track_visited_links, GTK_LABEL)

////////////////////////////////////////////////////////////////
//
//  text buffer
//

static cell *cgtk_text_buffer_new(cell *args) {
    arg0(args); // TODO optional GtkTextTagTable *table);
    GtkTextBuffer *textbuf;
    textbuf = gtk_text_buffer_new(NULL);
    return cell_special(magic_gtk_textbuf, (void *)textbuf);
}

/* TODO
gint gtk_text_buffer_get_line_count (textbuf)
gint gtk_text_buffer_get_char_count ()
gtk_text_buffer_get_tag_table ()
*/

// BUG: iter is where?
static cell *cgtk_text_buffer_insert_at_cursor(cell *tb, cell *str) {
    GtkTextBuffer *textbuf;
    char_t *str_ptr;
    index_t str_len;
    if (!get_gtktextbuffer(tb, &textbuf, str)
     || !peek_string(str, &str_ptr, &str_len, NIL)) {
        return cell_error();
    }
#if 0
    // TODO iter
    GtkTextIter *iter = NULL;
    gtk_text_buffer_insert(textbuf, iter, str_ptr, str_len);
#endif
    gtk_text_buffer_insert_at_cursor(textbuf, str_ptr, str_len);

    return cell_void();
}

/* TODO
gtk_text_buffer_insert_interactive ()
gtk_text_buffer_insert_interactive_at_cursor ()
...
*/

////////////////////////////////////////////////////////////////
//
//  text view
//

static cell *cgtk_text_view_new(cell *args) {
    cell *a;
    GtkWidget *text_view;
    if (list_pop(&args, &a)) {
        GtkTextBuffer *textbuf;
        if (!get_gtktextbuffer(a, &textbuf, NIL)) {
            return cell_error();
        }
        arg0(args);
        text_view = gtk_text_view_new_with_buffer(textbuf);
    } else {
        text_view = gtk_text_view_new();
    }
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
    // TODO sync with io and so on
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

static cell *cgtk_println(cell *args) {
    cell *result = cgtk_print(args);
    g_print("\n");
    return result;
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
        return cell_error();
    }
    if (!peek_special((const char *)0, app, (void **)&gp, callback)) {
        cell_unref(hook); // TODO
        return cell_error();
    }
    if (!get_symbol(hook, &signal, callback)) {
        cell_unref(app);
        return cell_error();
    }
    if (app->_.special.magic != magic_gtk_app
     && app->_.special.magic != magic_gtk_widget) {
        cell_unref(error_rt1("not a widget nor application", cell_ref(app)));
        cell_unref(app);
        return cell_error();
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
        return cell_error();
    }
    gtk_widget_destroy(wp);
    // properly invalidate widget to avoid future refs to dead gtk widget
    widget->_.special.ptr = NULL;
    widget->_.special.magic = NULL;
    cell_unref(widget);
    return cell_void();
}

WIDGET_VOID(widget_show_all, GTK_WIDGET);
WIDGET_VOID(widget_show, GTK_WIDGET);
WIDGET_VOID(widget_show_now, GTK_WIDGET);
WIDGET_VOID(widget_hide, GTK_WIDGET);
WIDGET_GET_BOOL(widget_activate, GTK_WIDGET);
WIDGET_GET_BOOL(widget_is_focus, GTK_WIDGET);
WIDGET_VOID(widget_grab_focus, GTK_WIDGET);
WIDGET_VOID(widget_grab_default, GTK_WIDGET);
WIDGET_SET_CSTRING(widget_set_name, GTK_WIDGET);

// TODO widget_draw

////////////////////////////////////////////////////////////////
//
//  window
//  https://developer.gnome.org/gtk3/stable/GtkWindow.html
//

WIDGET_SET_CSTRING(window_set_title, GTK_WINDOW);
WIDGET_SET_2INT(window_set_default_size, GTK_WINDOW);

////////////////////////////////////////////////////////////////
//

cell *module_gtk() {
    // TODO consider cache
    cell *a = cell_assoc();

    // TODO these functions are mostly impure

    DEFINE_CFUNN(application_new)
    DEFINE_CFUN2(application_run)
    DEFINE_CFUN1(application_window_new)
    DEFINE_CFUNN(button_new)   // also: button_new_with_label
    DEFINE_CFUNN(button_box_new)
    DEFINE_CFUN1(button_clicked)
    DEFINE_CFUN1(button_get_label)
    DEFINE_CFUN2(button_set_label)
    DEFINE_CFUN1(button_get_use_underline)
    DEFINE_CFUN2(button_set_use_underline)
    DEFINE_CFUN2(button_set_image)
    DEFINE_CFUN1(button_get_image)
    DEFINE_CFUN1(button_get_always_show_image)
    DEFINE_CFUN2(button_set_always_show_image)
    DEFINE_CFUN2(container_add)
    DEFINE_CFUN2(container_remove)
    DEFINE_CFUNN(container_check_resize)
    DEFINE_CFUN1(container_get_focus_child)
    DEFINE_CFUN2(container_set_focus_child)
    DEFINE_CFUN1(container_get_border_width)
    DEFINE_CFUN2(container_set_border_width)
    DEFINE_CFUNN(grid_new)
    DEFINE_CFUNN(grid_attach)
    DEFINE_CFUN1(image_new_from_file)
    DEFINE_CFUN2(image_set_from_file)
    DEFINE_CFUN2(image_set_from_resource)
    DEFINE_CFUN1(image_clear)
    DEFINE_CFUNN(image_new)
    DEFINE_CFUN2(image_set_pixel_size)
    DEFINE_CFUN1(image_get_pixel_size)
    DEFINE_CFUN1(init)
    DEFINE_CFUNN(main)
    DEFINE_CFUN1(label_new)
    DEFINE_CFUN2(label_set_text)
    DEFINE_CFUN2(label_set_markup)
    DEFINE_CFUN2(label_set_markup_with_mnemonic)
    DEFINE_CFUN2(label_set_pattern)
    DEFINE_CFUN2(label_set_xalign)
    DEFINE_CFUN2(label_set_yalign)
    DEFINE_CFUN2(label_set_width_chars)
    DEFINE_CFUN2(label_set_max_width_chars)
    DEFINE_CFUN2(label_set_line_wrap)
    DEFINE_CFUN2(label_set_lines)
    DEFINE_CFUN1(label_get_mnemonic_keyval)
    DEFINE_CFUN1(label_get_selectable)
    DEFINE_CFUN1(label_get_text)
    DEFINE_CFUNN(label_select_region)
    DEFINE_CFUN2(label_set_selectable)
    DEFINE_CFUN2(label_set_text_with_mnemonic)
    DEFINE_CFUN1(label_get_xalign)
    DEFINE_CFUN1(label_get_yalign)
    DEFINE_CFUN1(label_get_width_chars)
    DEFINE_CFUN1(label_get_max_width_chars)
    DEFINE_CFUN1(label_get_label)
    DEFINE_CFUN1(label_get_lines)
    DEFINE_CFUN1(label_get_lines)
    DEFINE_CFUN1(label_get_use_markup)
    DEFINE_CFUN1(label_get_use_underline)
    DEFINE_CFUN1(label_get_single_line_mode)
    DEFINE_CFUN1(label_get_angle)
    DEFINE_CFUN2(label_set_label)
    DEFINE_CFUN2(label_set_use_markup)
    DEFINE_CFUN2(label_set_use_underline)
    DEFINE_CFUN2(label_set_single_line_mode)
    DEFINE_CFUN2(label_set_angle)
    DEFINE_CFUN1(label_get_current_uri)
    DEFINE_CFUN2(label_set_track_visited_links)
    DEFINE_CFUN1(label_get_track_visited_links)
    DEFINE_CFUNN(print)
    DEFINE_CFUNN(println)
    DEFINE_CFUNN(signal_connect)
    DEFINE_CFUNN(text_buffer_new)
    DEFINE_CFUN2(text_buffer_insert_at_cursor)
    DEFINE_CFUNN(text_view_new)
    DEFINE_CFUN1(widget_destroy)
    DEFINE_CFUN1(widget_show_all)
    DEFINE_CFUN1(widget_show)
    DEFINE_CFUN1(widget_show_now)
    DEFINE_CFUN1(widget_hide)
    DEFINE_CFUN1(widget_activate)
    DEFINE_CFUN1(widget_is_focus)
    DEFINE_CFUN1(widget_grab_focus)
    DEFINE_CFUN1(widget_grab_default)
    DEFINE_CFUN2(widget_set_name)
    DEFINE_CFUN2(window_set_title)
    DEFINE_CFUNN(window_set_default_size)

    return a;
}

