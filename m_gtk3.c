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
#include "oblist.h"
#include "run.h"
#include "err.h"

////////////////////////////////////////////////////////////////
//
//  magic types

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
//  'set' types
//  TODO improve
//

// enum GtkOrientation
static cell *cgtk_orientation_horizontal;
static cell *cgtk_orientation_vertical;

// enum GtkWindowType
static cell *cgtk_window_toplevel;
static cell *cgtk_window_popup;

////////////////////////////////////////////////////////////////
//
//  macros
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

#define WIDGET_SET_INT_INT(gname, gtype) \
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

#define WIDGET_SET_CSTRING_INT_GET_WIDGET(gname, gtype) \
static cell *cgtk_##gname(cell *args) { \
    cell *widget; \
    cell *str; \
    cell *val; \
    GtkWidget *wp; \
    integer_t ival; \
    char_t *cstr; \
    GtkWidget *result; \
    if (!arg3(args, &widget, &str, &val)) return cell_error(); \
    if (!get_gtkwidget(widget, &wp, str)) { \
        cell_unref(val); \
        return cell_error(); \
    } \
    if (!peek_cstring(str, &cstr, str) \
     || !get_integer(val, &ival, NIL)) return cell_error(); \
    result = gtk_##gname(gtype(wp), cstr, ival); \
    return cell_special(magic_gtk_widget, (void *)result); \
}

#define VOID_GET_WIDGET(gname) \
static cell *cgtk_##gname(cell *args) { \
    GtkWidget *result; \
    arg0(args); \
    result = gtk_##gname(); \
    return cell_special(magic_gtk_widget, (void *)result); \
}

#define VOID_SET_BOOL(gname) \
static cell *cgtk_##gname(cell *val) { \
    int bval = 0; \
    if (!get_boolean(val, &bval, NIL)) return cell_error(); \
    gtk_##gname(bval); \
    return cell_void(); \
}

#define VOID_GET_BOOL(gname) \
static cell *cgtk_##gname(cell *args) { \
    gboolean bval; \
    arg0(args); \
    bval = gtk_##gname(); \
    return cell_ref(bval ? hash_t : hash_f); \
}

#define VOID_SET_CSTRING(gname) \
static cell *cgtk_##gname(cell *str) { \
    char_t *cstr; \
    if (!peek_cstring(str, &cstr, NIL)) return cell_error(); \
    gtk_##gname(cstr); \
    cell_unref(str); \
    return cell_void(); \
}

#define VOID_GET_CSTRING(gname) \
static cell *cgtk_##gname(cell *args) { \
    const char *cstr; \
    arg0(args); \
    cstr = gtk_##gname(); \
    return cell_astring(strdup(cstr), strlen(cstr)); \
}

#define CSTRING_GET_WIDGET(gname) \
static cell *cgtk_##gname(cell *str) { \
    GtkWidget *result; \
    char_t *cstr; \
    if (!peek_cstring(str, &cstr, NIL)) return cell_error(); \
    result = gtk_##gname(cstr); \
    cell_unref(str); \
    return cell_special(magic_gtk_widget, (void *)result); \
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

static cell *cgtk_button_box_new(cell *typ) {
    GtkOrientation t;
    GtkWidget *button_box;

    if (typ == cgtk_orientation_horizontal) t = GTK_ORIENTATION_HORIZONTAL;
    else if (typ == cgtk_orientation_vertical) t = GTK_ORIENTATION_VERTICAL;
    else return error_rt1("orientation must be 'horizontal or 'vertical", typ);
    cell_unref(typ);
    button_box = gtk_button_box_new(t);
    return cell_special(magic_gtk_widget, (void *)button_box);
}

////////////////////////////////////////////////////////////////
//
//  button
//  https://developer.gnome.org/gtk3/stable/GtkButton.html
//

VOID_GET_WIDGET(button_new)
CSTRING_GET_WIDGET(button_new_with_label)
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
//  dialog
//  https://developer.gnome.org/gtk3/stable/GtkDialog.html
//

VOID_GET_WIDGET(dialog_new)
// TODO gtk_dialog_new_with_buttons ()

WIDGET_VOID(dialog_run, GTK_DIALOG)
WIDGET_SET_INT(dialog_response, GTK_DIALOG)

WIDGET_SET_CSTRING_INT_GET_WIDGET(dialog_add_button, GTK_DIALOG)

// TODO dialog_add_buttons
// TODO ialog_add_action_widget

WIDGET_SET_INT(dialog_set_default_response, GTK_DIALOG)

// TODO gtk_dialog_get_response_for_widget
// TODO dialog_get_widget_for_response
// TODO gtk_dialog_get_content_area
// TODO gtk_dialog_get_header_bar

////////////////////////////////////////////////////////////////
//
//  grid
//  https://developer.gnome.org/gtk3/stable/GtkGrid.html
//

VOID_GET_WIDGET(grid_new)

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
WIDGET_SET_INT(grid_insert_row, GTK_GRID)
WIDGET_SET_INT(grid_insert_column, GTK_GRID)
WIDGET_SET_INT(grid_remove_row, GTK_GRID)
WIDGET_SET_INT(grid_remove_column, GTK_GRID)
// TODO gtk_grid_insert_next_to ()
WIDGET_SET_BOOL(grid_set_row_homogeneous, GTK_GRID)
WIDGET_GET_BOOL(grid_get_row_homogeneous, GTK_GRID)
WIDGET_SET_INT(grid_set_row_spacing, GTK_GRID)
WIDGET_GET_INT(grid_get_row_spacing, GTK_GRID)
WIDGET_SET_BOOL(grid_set_column_homogeneous, GTK_GRID)
WIDGET_GET_BOOL(grid_get_column_homogeneous, GTK_GRID)
WIDGET_SET_INT(grid_set_column_spacing, GTK_GRID)
WIDGET_GET_INT(grid_get_column_spacing, GTK_GRID)
WIDGET_SET_INT(grid_set_baseline_row, GTK_GRID)
WIDGET_GET_INT(grid_get_baseline_row, GTK_GRID)
// TODO grid_set_row_baseline_position
// TODO grid_get_row_baseline_position


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

CSTRING_GET_WIDGET(image_new_from_file)

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
VOID_GET_WIDGET(image_new)
WIDGET_SET_INT(image_set_pixel_size, GTK_IMAGE)
WIDGET_GET_INT(image_get_pixel_size, GTK_IMAGE)

////////////////////////////////////////////////////////////////
//
//  label
//  https://developer.gnome.org/gtk3/stable/GtkLabel.html
//

static cell *cgtk_label_new(cell *str) {
    GtkWidget *label;
    char_t *cstr;
    if (!peek_cstring(str, &cstr, NIL)) return cell_error();
    label = gtk_label_new(cstr);
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
WIDGET_SET_INT_INT(label_select_region, GTK_LABEL)
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
     || !peek_string(str, &str_ptr, &str_len, NIL)) return cell_error();
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
//  callbacks
//
//  TODO callbacks may be a separate thread?
//
//  TODO look at g_signal_handler_disconnect() or
//       g_signal_handlers_disconnect_by_func() to manage memory
//

//

// callback, no extra argument provided is program to run
static void do_callback_none(GtkApplication* gp, gpointer data) {

 // assert(cell_is_func((cell *)data));
 // assert(cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_app)
 //     || cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_widget));
 // assert((GtkApplication *) (cell_car(cell_cdr((cell *)data))->_.special.ptr) == gp);
 // printf("\n***callback***\n");

    run_async(cell_ref((cell *)data));
}

// callback, argument provided is an int
static void do_callback_int(GtkApplication* gp, gint extra, gpointer data) {
 // printf("\n***callback int***\n");

    // TODO this function is in principle async
    // TODO the extra data should be provided to the program somehow

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
    if (!peek_symbol(hook, &signal, callback)) {
        cell_unref(app);
        return cell_error();
    }

    if (app->_.special.magic != magic_gtk_app
     && app->_.special.magic != magic_gtk_widget) {
        cell_unref(error_rt1("not a widget nor application", cell_ref(app)));
        cell_unref(app);
        cell_unref(hook);
        return cell_error();
    }
    cell *callbackprog = NIL;

    // TODO be more efficient
    if (strcmp(signal, "activate") == 0 || strcmp(signal, "clicked") == 0) {
        // no extra data provided
        // connect where data is the function with one argument, the current argument (app or widget)
        callbackprog = compile(cell_func(callback, cell_list(app, NIL)));

        g_signal_connect(gp, signal, G_CALLBACK(do_callback_none), callbackprog);


    } else if (strcmp(signal, "response") == 0) {
        // extra parameter: integer response_id
        // TODO do something smarter here
        callbackprog = compile(cell_func(callback, cell_list(app, NIL)));

        g_signal_connect(gp, signal, G_CALLBACK(do_callback_int), callbackprog);
    } else {
        cell_unref(app);
        return error_rt1("signal not (yet) supported", hook);
    }

    // cell_unref(callbackprog); // TODO when to unref callbackprog ???

    cell_unref(app);
    cell_unref(hook);
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
    // TODO sure?
    widget->_.special.ptr = NULL;
    widget->_.special.magic = NULL;
    cell_unref(widget);
    return cell_void();
}

WIDGET_GET_BOOL(widget_in_destruction, GTK_WIDGET)
// TODO gtk_widget_destroyed
WIDGET_VOID(widget_show, GTK_WIDGET)
WIDGET_VOID(widget_show_now, GTK_WIDGET)
WIDGET_VOID(widget_hide, GTK_WIDGET)
// TODO gtk_widget_draw
// TODO gtk_widget_queue_draw
WIDGET_VOID(widget_show_all, GTK_WIDGET)
// TODO gtk_widget_get_frame_clock
WIDGET_GET_INT(widget_get_scale_factor, GTK_WIDGET)
// TODO gtk_widget_add_tick_callback
// TODO gtk_widget_remove_tick_callback
// TODO gtk_widget_add_accelerator
// TODO gtk_widget_remove_accelerator
// TODO ...
WIDGET_GET_BOOL(widget_activate, GTK_WIDGET)
// TODO gtk_widget_intersect
WIDGET_GET_BOOL(widget_is_focus, GTK_WIDGET)
WIDGET_VOID(widget_grab_focus, GTK_WIDGET)
WIDGET_VOID(widget_grab_default, GTK_WIDGET)
WIDGET_SET_CSTRING(widget_set_name, GTK_WIDGET)
WIDGET_GET_CSTRING(widget_get_name, GTK_WIDGET)
// TODO widget_set_sensitive
// TODO gtk_widget_set_parent_window
// TODO gtk_widget_get_parent_window
WIDGET_SET_INT(widget_set_events, GTK_WIDGET)
WIDGET_GET_INT(widget_get_events, GTK_WIDGET)
WIDGET_SET_INT(widget_add_events, GTK_WIDGET)
// TODO gtk_widget_set_device_events
// TODO gtk_widget_get_device_events
// TODO gtk_widget_add_device_events ()
// TODO gtk_widget_set_device_enabled
// TODO gtk_widget_get_device_enabled
WIDGET_GET_WIDGET(widget_get_toplevel, GTK_WIDGET)
// TODO gtk_widget_get_ancestor
// TODO gtk_widget_get_visual
// TODO gtk_widget_set_visual
// TODO gtk_widget_is_ancestor
// TODO gtk_widget_translate_coordinates
WIDGET_GET_BOOL(widget_hide_on_delete, GTK_WIDGET)
// TODO gtk_widget_set_direction
// TODO gtk_widget_get_direction
// TODO gtk_widget_set_default_direction
// TODO gtk_widget_get_default_direction
// TODO gtk_widget_shape_combine_region
// TODO gtk_widget_input_shape_combine_region
// TODO gtk_widget_create_pango_context
// TODO gtk_widget_get_pango_context
// TODO gtk_widget_set_font_options
// TODO gtk_widget_get_font_options
// TODO gtk_widget_set_font_map
// TODO gtk_widget_get_font_map
// TODO gtk_widget_create_pango_layout
// TODO gtk_widget_queue_draw_area
// TODO gtk_widget_queue_draw_region
// TODO gtk_widget_set_app_paintable
WIDGET_SET_BOOL(widget_set_redraw_on_allocate, GTK_WIDGET)
// TODO gtk_widget_mnemonic_activate
// TODO gtk_widget_class_install_style_property
// TODO gtk_widget_class_install_style_property_parser
// TODO gtk_widget_class_find_style_property
// TODO gtk_widget_class_list_style_properties
// TODO gtk_widget_send_focus_change
// gtk_widget_style_get ()
// gtk_widget_style_get_property ()
// gtk_widget_class_set_accessible_type
// gtk_widget_class_set_accessible_role
// gtk_widget_get_accessible
// TODO gtk_widget_child_focus ()
// TODO gtk_widget_child_notify ()
// TODO gtk_widget_freeze_child_notify ()
WIDGET_GET_WIDGET(widget_get_parent, GTK_WIDGET)
// TODO gtk_widget_get_settings()
// TODO gtk_widget_get_clipboard()
// TODO gtk_widget_get_display()
// TODO gtk_widget_get_screen()
WIDGET_GET_BOOL(widget_has_screen, GTK_WIDGET)
// TODO gtk_widget_get_size_request
// TODO gtk_widget_set_size_request
// TODO gtk_widget_thaw_child_notify
WIDGET_SET_BOOL(widget_set_no_show_all, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_no_show_all, GTK_WIDGET)
// TODO gtk_widget_list_mnemonic_labels
WIDGET_SET_WIDGET(widget_add_mnemonic_label, GTK_WIDGET)
WIDGET_SET_WIDGET(widget_remove_mnemonic_label, GTK_WIDGET)
WIDGET_VOID(widget_error_bell, GTK_WIDGET)
// TODO gtk_widget_keynav_failed
WIDGET_GET_CSTRING(widget_get_tooltip_markup, GTK_WIDGET)
WIDGET_SET_CSTRING(widget_set_tooltip_markup, GTK_WIDGET)
WIDGET_GET_CSTRING(widget_get_tooltip_text, GTK_WIDGET)
WIDGET_SET_CSTRING(widget_set_tooltip_text, GTK_WIDGET)
// TODO widget_get_tooltip_window
// TODO widget_set_tooltip_window
WIDGET_GET_BOOL(widget_get_has_tooltip, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_has_tooltip, GTK_WIDGET)
WIDGET_VOID(widget_trigger_tooltip_query, GTK_WIDGET)
// TODO gtk_widget_get_window
// TODO gtk_widget_register_window
// TODO gtk_widget_unregister_window
// TODO gtk_cairo_should_draw_window
// TODO gtk_cairo_transform_to_window
WIDGET_GET_INT(widget_get_allocated_width, GTK_WIDGET)
WIDGET_GET_INT(widget_get_allocated_height, GTK_WIDGET)
// TODO gtk_widget_get_allocation
// TODO gtk_widget_set_allocation
// TODO gtk_widget_get_allocated_baseline
// TODO gtk_widget_get_allocated_size
// TODO gtk_widget_get_clip
// TODO gtk_widget_set_clip
// TODO gtk_widget_get_app_paintable
WIDGET_GET_BOOL(widget_get_can_default, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_can_default, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_can_focus, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_can_focus, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_focus_on_click, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_focus_on_click, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_has_window, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_has_window, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_sensitive, GTK_WIDGET)
WIDGET_GET_BOOL(widget_is_sensitive, GTK_WIDGET)
// TODO gtk_widget_get_state
WIDGET_GET_BOOL(widget_get_visible, GTK_WIDGET)
WIDGET_GET_BOOL(widget_is_visible, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_visible, GTK_WIDGET)
// TODO gtk_widget_set_state_flags
// TODO gtk_widget_unset_state_flags
// TODO gtk_widget_get_state_flags
WIDGET_GET_BOOL(widget_has_default, GTK_WIDGET)
WIDGET_GET_BOOL(widget_has_focus, GTK_WIDGET)
WIDGET_GET_BOOL(widget_has_visible_focus, GTK_WIDGET)
WIDGET_GET_BOOL(widget_has_grab, GTK_WIDGET)
WIDGET_GET_BOOL(widget_is_drawable, GTK_WIDGET)
WIDGET_GET_BOOL(widget_is_toplevel, GTK_WIDGET)
// TODO gtk_widget_set_window
WIDGET_SET_BOOL(widget_set_receives_default, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_receives_default, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_support_multidevice, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_support_multidevice, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_realized, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_realized, GTK_WIDGET)
WIDGET_SET_BOOL(widget_set_mapped, GTK_WIDGET)
WIDGET_GET_BOOL(widget_get_mapped, GTK_WIDGET)
// TODO widget_device_is_shadowed
// TODO gtk_widget_get_modifier_mask
// TODO gtk_widget_insert_action_group
WIDGET_GET_FLOAT(widget_get_opacity, GTK_WIDGET)
WIDGET_SET_FLOAT(widget_set_opacity, GTK_WIDGET)
// TODO gtk_widget_list_action_prefixes
// TODO gtk_widget_get_action_group
// TODO gtk_widget_get_path
// TODO gtk_widget_get_style_context
WIDGET_VOID(widget_reset_style, GTK_WIDGET)
// TODO widget_class_get_css_name, GTK_WIDGET_CLASS
// TODO widget_class_set_css_name, GTK_WIDGET_CLASS
// TODO gtk_requisition_new
// TODO gtk_requisition_copy
// TODO gtk_requisition_free
// TODO gtk_widget_get_preferred_height
// TODO gtk_widget_get_preferred_width
// TODO gtk_widget_get_preferred_height_for_width
// TODO gtk_widget_get_preferred_width_for_height
// TODO gtk_widget_get_preferred_height_and_baseline_for_width
// TODO gtk_widget_get_request_mode
// TODO gtk_widget_get_preferred_size
// TODO gtk_distribute_natural_allocation
// TODO gtk_widget_get_halign
// TODO gtk_widget_set_halign
// TODO gtk_widget_get_valign
// TODO gtk_widget_get_valign_with_baseline
// TODO gtk_widget_set_valign_with_baseline
// TODO gtk_widget_set_valign
// ....
// TODO gtk_widget_get_margin_start
// TODO gtk_widget_set_margin_start
// TODO gtk_widget_get_margin_end
// TODO gtk_widget_set_margin_end
// ....


// TODO widget_draw

////////////////////////////////////////////////////////////////
//
//  window
//  https://developer.gnome.org/gtk3/stable/GtkWindow.html
//

static cell *cgtk_window_new(cell *typ) {
    GtkWindowType t;
    GtkWidget *window;

    if (typ == cgtk_window_toplevel) t = GTK_WINDOW_TOPLEVEL;
    else if (typ == cgtk_window_popup) t = GTK_WINDOW_POPUP;
    else return error_rt1("window type must be 'toplevel or 'popup", typ);
    cell_unref(typ);
    window = gtk_window_new(t);
    return cell_special(magic_gtk_widget, (void *)window);
}

WIDGET_SET_CSTRING(window_set_title, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_resizable, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_resizable, GTK_WINDOW)
// TODO  window_add_accel_group
// TODO  window_remove_accel_group
WIDGET_GET_BOOL(window_activate_focus, GTK_WINDOW)
WIDGET_GET_BOOL(window_activate_default, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_modal, GTK_WINDOW)
WIDGET_SET_INT_INT(window_set_default_size, GTK_WINDOW)
// TODO gtk_window_set_geometry_hints ()
// TODO gtk_window_set_gravity ()
// TODO gtk_window_get_gravity ()
// TODO gtk_window_set_position ()
// TODO gtk_window_set_transient_for
// TODO gtk_window_set_attached_to
WIDGET_SET_BOOL(window_set_destroy_with_parent, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_hide_titlebar_when_maximized, GTK_WINDOW)
// TODO gtk_window_set_screen
// TODO gtk_window_get_screen
WIDGET_GET_BOOL(window_is_active, GTK_WINDOW)
WIDGET_GET_BOOL(window_is_maximized, GTK_WINDOW)
WIDGET_GET_BOOL(window_has_toplevel_focus, GTK_WINDOW)
// TODO gtk_window_list_toplevels
// TODO gtk_window_add_mnemonic
// TODO gtk_window_remove_mnemonic
// TODO gtk_window_mnemonic_activate
// TODO gtk_window_activate_key
// TODO gtk_window_propagate_key_event
WIDGET_GET_WIDGET(window_get_focus, GTK_WINDOW)
WIDGET_SET_WIDGET(window_set_focus, GTK_WINDOW)
WIDGET_GET_WIDGET(window_get_default_widget, GTK_WINDOW)
WIDGET_SET_WIDGET(window_set_default, GTK_WINDOW)
WIDGET_VOID(window_present, GTK_WINDOW)
// TODO WIDGET_TIMESTAMP(window_present_with_time, GTK_WINDOW)
WIDGET_VOID(window_close, GTK_WINDOW)
WIDGET_VOID(window_iconify, GTK_WINDOW)
WIDGET_VOID(window_deiconify, GTK_WINDOW)
WIDGET_VOID(window_stick, GTK_WINDOW)
WIDGET_VOID(window_unstick, GTK_WINDOW)
WIDGET_VOID(window_maximize, GTK_WINDOW)
WIDGET_VOID(window_unmaximize, GTK_WINDOW)
WIDGET_VOID(window_fullscreen, GTK_WINDOW)
// TODO gtk_window_fullscreen_on_monitor
WIDGET_VOID(window_unfullscreen, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_keep_above, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_keep_below, GTK_WINDOW)
// TODO gtk_window_begin_resize_drag
// TODO gtk_window_begin_move_drag
WIDGET_SET_BOOL(window_set_decorated, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_deletable, GTK_WINDOW)
// TODO gtk_window_set_mnemonic_modifier
// TODO gtk_window_set_type_hint
WIDGET_SET_BOOL(window_set_skip_taskbar_hint, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_skip_pager_hint, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_urgency_hint, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_accept_focus, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_focus_on_map, GTK_WINDOW)
WIDGET_SET_CSTRING(window_set_startup_id, GTK_WINDOW)
WIDGET_SET_CSTRING(window_set_role, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_decorated, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_deletable, GTK_WINDOW)
// gtk_window_get_default_icon_list
VOID_GET_CSTRING(window_get_default_icon_name)
// gtk_window_get_default_size
WIDGET_GET_BOOL(window_get_destroy_with_parent, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_hide_titlebar_when_maximized, GTK_WINDOW)
// TODO window_get_icon
// TODO window_get_icon_list
WIDGET_GET_CSTRING(window_get_icon_name, GTK_WINDOW)
// gtk_window_get_mnemonic_modifier
WIDGET_GET_BOOL(window_get_modal, GTK_WINDOW)
// gtk_window_get_position
WIDGET_GET_CSTRING(window_get_role, GTK_WINDOW)
// gtk_window_get_size
WIDGET_GET_CSTRING(window_get_title, GTK_WINDOW)
//  window_get_title, GTK_WINDOW) -> GTK_WINDOW
WIDGET_GET_WIDGET(window_get_attached_to, GTK_WINDOW)
// TODO gtk_window_get_type_hint
WIDGET_GET_BOOL(window_get_skip_taskbar_hint, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_skip_pager_hint, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_urgency_hint, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_accept_focus, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_focus_on_map, GTK_WINDOW)
// TODO gtk_window_get_group
WIDGET_GET_BOOL(window_has_group, GTK_WINDOW)
// TODO gtk_window_get_window_type
WIDGET_SET_INT_INT(window_move, GTK_WINDOW)
WIDGET_SET_INT_INT(window_resize, GTK_WINDOW)
// TODO    window_set_default_icon_list
// TODO    window_set_default_icon
// TODO window_set_default_icon_from_file
VOID_SET_CSTRING(window_set_default_icon_name)
// TODO window_set_icon
// TODO window_set_icon_list
// TODO window_set_icon_from_file
WIDGET_SET_CSTRING(window_set_icon_name, GTK_WINDOW)
VOID_SET_BOOL(window_set_auto_startup_notification)
WIDGET_GET_BOOL(window_get_mnemonics_visible, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_mnemonics_visible, GTK_WINDOW)
WIDGET_GET_BOOL(window_get_focus_visible, GTK_WINDOW)
WIDGET_SET_BOOL(window_set_focus_visible, GTK_WINDOW)
// TODO window_get_application
// TODO window_set_application
WIDGET_SET_BOOL(window_set_has_user_ref_count, GTK_WINDOW)
WIDGET_SET_WIDGET(window_set_titlebar, GTK_WINDOW)
WIDGET_GET_WIDGET(window_get_titlebar, GTK_WINDOW)
VOID_SET_BOOL(window_set_interactive_debugging)

////////////////////////////////////////////////////////////////
//

cell *module_gtk() {
    // TODO consider cache
    cell *a = cell_assoc();

    // TODO these functions are mostly impure

    DEFINE_CFUNN(application_new)
    DEFINE_CFUN2(application_run)
    DEFINE_CFUN1(application_window_new)
    DEFINE_CFUNN(button_new)
    DEFINE_CFUNN(button_new_with_label)
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
    DEFINE_CFUNN(dialog_new)
    DEFINE_CFUN1(dialog_run)
    DEFINE_CFUN2(dialog_response)
    DEFINE_CFUNN(dialog_add_button)
    DEFINE_CFUN2(dialog_set_default_response)
    DEFINE_CFUNN(grid_new)
    DEFINE_CFUNN(grid_attach)
    // TODO grid_attach_next_to
    // TODO_grid_get_child_at
    DEFINE_CFUN2(grid_insert_row)
    DEFINE_CFUN2(grid_insert_column)
    DEFINE_CFUN2(grid_remove_row)
    DEFINE_CFUN2(grid_remove_column)
    // TODO grid_insert_next_to
    DEFINE_CFUN2(grid_set_row_homogeneous)
    DEFINE_CFUN1(grid_get_row_homogeneous)
    DEFINE_CFUN2(grid_set_row_spacing)
    DEFINE_CFUN1(grid_get_row_spacing)
    DEFINE_CFUN2(grid_set_column_homogeneous)
    DEFINE_CFUN1(grid_get_column_homogeneous)
    DEFINE_CFUN2(grid_set_column_spacing)
    DEFINE_CFUN1(grid_get_column_spacing)
    DEFINE_CFUN2(grid_set_baseline_row)
    DEFINE_CFUN1(grid_get_baseline_row)
    // TODO DEFINE_CFUNN(grid_set_row_baseline_position)
    // TODO DEFINE_CFUN2(grid_get_row_baseline_position)
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
    DEFINE_CFUN1(widget_in_destruction)
    DEFINE_CFUN1(widget_show)
    DEFINE_CFUN1(widget_show_now)
    DEFINE_CFUN1(widget_hide)
    DEFINE_CFUN1(widget_show_all)
    DEFINE_CFUN1(widget_get_scale_factor)
    DEFINE_CFUN1(widget_activate)
    DEFINE_CFUN1(widget_is_focus)
    DEFINE_CFUN1(widget_grab_focus)
    DEFINE_CFUN1(widget_grab_default)
    DEFINE_CFUN2(widget_set_name)
    DEFINE_CFUN1(widget_get_name)
    DEFINE_CFUN2(widget_set_events)
    DEFINE_CFUN1(widget_get_events)
    DEFINE_CFUN2(widget_add_events)
    DEFINE_CFUN1(widget_get_toplevel)
    DEFINE_CFUN1(widget_hide_on_delete)
    DEFINE_CFUN2(widget_set_redraw_on_allocate)
    DEFINE_CFUN1(widget_get_parent)
    DEFINE_CFUN1(widget_has_screen)
    DEFINE_CFUN2(widget_set_no_show_all)
    DEFINE_CFUN1(widget_get_no_show_all)
    DEFINE_CFUN2(widget_add_mnemonic_label)
    DEFINE_CFUN2(widget_remove_mnemonic_label)
    DEFINE_CFUN1(widget_error_bell)
    DEFINE_CFUN1(widget_get_tooltip_markup)
    DEFINE_CFUN2(widget_set_tooltip_markup)
    DEFINE_CFUN1(widget_get_tooltip_text)
    DEFINE_CFUN2(widget_set_tooltip_text)
    DEFINE_CFUN1(widget_get_has_tooltip)
    DEFINE_CFUN2(widget_set_has_tooltip)
    DEFINE_CFUN1(widget_trigger_tooltip_query)
    DEFINE_CFUN1(widget_get_allocated_width)
    DEFINE_CFUN1(widget_get_allocated_height)
    DEFINE_CFUN1(widget_get_can_default)
    DEFINE_CFUN2(widget_set_can_default)
    DEFINE_CFUN1(widget_get_can_focus)
    DEFINE_CFUN2(widget_set_can_focus)
    DEFINE_CFUN1(widget_get_focus_on_click)
    DEFINE_CFUN2(widget_set_focus_on_click)
    DEFINE_CFUN1(widget_get_has_window)
    DEFINE_CFUN2(widget_set_has_window)
    DEFINE_CFUN1(widget_get_sensitive)
    DEFINE_CFUN1(widget_is_sensitive)
    DEFINE_CFUN1(widget_get_visible)
    DEFINE_CFUN1(widget_is_visible)
    DEFINE_CFUN2(widget_set_visible)
    DEFINE_CFUN1(widget_has_default)
    DEFINE_CFUN1(widget_has_focus)
    DEFINE_CFUN1(widget_has_visible_focus)
    DEFINE_CFUN1(widget_has_grab)
    DEFINE_CFUN1(widget_is_drawable)
    DEFINE_CFUN1(widget_is_toplevel)
    DEFINE_CFUN2(widget_set_receives_default)
    DEFINE_CFUN1(widget_get_receives_default)
    DEFINE_CFUN2(widget_set_support_multidevice)
    DEFINE_CFUN1(widget_get_support_multidevice)
    DEFINE_CFUN2(widget_set_realized)
    DEFINE_CFUN1(widget_get_realized)
    DEFINE_CFUN2(widget_set_mapped)
    DEFINE_CFUN1(widget_get_mapped)
    DEFINE_CFUN1(widget_get_opacity)
    DEFINE_CFUN2(widget_set_opacity)
    DEFINE_CFUN1(widget_reset_style)
    DEFINE_CFUN1(window_new)
    DEFINE_CFUN2(window_set_title)
    DEFINE_CFUN2(window_set_resizable)
    DEFINE_CFUN1(window_get_resizable)
    // TODO window_add_accel_group
    // TODO window_remove_accel_group
    DEFINE_CFUN1(window_activate_focus)
    DEFINE_CFUN1(window_activate_default)
    DEFINE_CFUN2(window_set_modal)
    DEFINE_CFUNN(window_set_default_size)
    // TODO window_set_geometry_hints
    // TODO window_set_gravity
    // TODO window_get_gravity
    // TODO window_set_position
    // TODO window_set_transient_for
    // TODO window_set_attached_to
    DEFINE_CFUN2(window_set_destroy_with_parent)
    DEFINE_CFUN2(window_set_hide_titlebar_when_maximized)
    // TODO window_set_screen
    // TODO window_get_screen
    DEFINE_CFUN1(window_is_active)
    DEFINE_CFUN1(window_is_maximized)
    DEFINE_CFUN1(window_has_toplevel_focus)
    // TODO window_list_toplevels
    // TODO window_add_mnemonic
    // TODO window_remove_mnemonic
    // TODO window_mnemonic_activate
    // TODO window_activate_key
    // TODO window_propagate_key_event
    DEFINE_CFUN1(window_get_focus)
    DEFINE_CFUN2(window_set_focus)
    DEFINE_CFUN1(window_get_default_widget)
    DEFINE_CFUN2(window_set_default)
    DEFINE_CFUN1(window_present)
    // TODO DEFINE_CFUN2(window_present_with_time)
    DEFINE_CFUN1(window_close)
    DEFINE_CFUN1(window_iconify)
    DEFINE_CFUN1(window_deiconify)
    DEFINE_CFUN1(window_stick)
    DEFINE_CFUN1(window_unstick)
    DEFINE_CFUN1(window_maximize)
    DEFINE_CFUN1(window_unmaximize)
    DEFINE_CFUN1(window_fullscreen)
    // TODO window_fullscreen_on_monitor
    DEFINE_CFUN1(window_unfullscreen)
    DEFINE_CFUN2(window_set_keep_above)
    DEFINE_CFUN2(window_set_keep_below)
    // TODO window_begin_resize_drag
    // TODO window_begin_move_drag
    DEFINE_CFUN2(window_set_decorated)
    DEFINE_CFUN2(window_set_deletable)
    // TODO window_set_mnemonic_modifier
    // TODO window_set_type_hint
    DEFINE_CFUN2(window_set_skip_taskbar_hint)
    DEFINE_CFUN2(window_set_skip_pager_hint)
    DEFINE_CFUN2(window_set_urgency_hint)
    DEFINE_CFUN2(window_set_accept_focus)
    DEFINE_CFUN2(window_set_focus_on_map)
    DEFINE_CFUN2(window_set_startup_id)
    DEFINE_CFUN2(window_set_role)
    DEFINE_CFUN1(window_get_decorated)
    DEFINE_CFUN1(window_get_deletable)
    // TODO window_get_default_icon_list
    DEFINE_CFUNN(window_get_default_icon_name)
    // TODO window_get_default_size
    DEFINE_CFUN1(window_get_destroy_with_parent)
    DEFINE_CFUN1(window_get_hide_titlebar_when_maximized)
    // TODO window_get_icon
    // TODO window_get_icon_list
    DEFINE_CFUN1(window_get_icon_name)
    // gtk_window_get_mnemonic_modifier
    DEFINE_CFUN1(window_get_modal)
    // gtk_window_get_position
    DEFINE_CFUN1(window_get_role)
    // gtk_window_get_size
    DEFINE_CFUN1(window_get_title)
    // window_get_title
    DEFINE_CFUN1(window_get_attached_to)
    // TODO window_get_type_hint
    DEFINE_CFUN1(window_get_skip_taskbar_hint)
    DEFINE_CFUN1(window_get_skip_pager_hint)
    DEFINE_CFUN1(window_get_urgency_hint)
    DEFINE_CFUN1(window_get_accept_focus)
    DEFINE_CFUN1(window_get_focus_on_map)
    // TODO window_get_group
    DEFINE_CFUN1(window_has_group)
    // TODO window_get_window_type
    DEFINE_CFUNN(window_move)
    DEFINE_CFUNN(window_resize)
    // TODO window_set_default_icon_list
    // TODO window_set_default_icon
    // TODO window_set_default_icon_from_file
    DEFINE_CFUN1(window_set_default_icon_name)
    // TODO window_set_icon
    // TODO window_set_icon_list
    // TODO window_set_icon_from_file
    DEFINE_CFUN2(window_set_icon_name)
    DEFINE_CFUN1(window_set_auto_startup_notification)
    DEFINE_CFUN1(window_get_mnemonics_visible)
    DEFINE_CFUN2(window_set_mnemonics_visible)
    DEFINE_CFUN1(window_get_focus_visible)
    DEFINE_CFUN2(window_set_focus_visible)
    // TODO window_get_application
    // TODO window_set_application
    DEFINE_CFUN2(window_set_has_user_ref_count)
    DEFINE_CFUN2(window_set_titlebar)
    DEFINE_CFUN1(window_get_titlebar)
    DEFINE_CFUN1(window_set_interactive_debugging)

    // Gtk enums
    cgtk_orientation_horizontal  = oblistv("horizontal", cell_ref(hash_undef));
    cgtk_orientation_vertical    = oblistv("vertical", cell_ref(hash_undef));
    cgtk_window_toplevel         = oblistv("toplevel", cell_ref(hash_undef));
    cgtk_window_popup            = oblistv("popup", cell_ref(hash_undef));

    return a;
}

