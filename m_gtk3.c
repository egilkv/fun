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
#include <math.h> // isfinite
#include <assert.h>

#include <gtk/gtk.h>

#include "cmod.h"
#include "number.h"
#include "compile.h"
#include "oblist.h"
#include "node.h" // newnode
#include "run.h"
#include "err.h"
#include "set.h"

static cell *cell_gdkevent(GdkEvent *event);

////////////////////////////////////////////////////////////////
//
//  magic types


static const char *magic_gtk_app(void *ptr) { 
    if (ptr) g_object_unref(ptr);
    return "gtk_application"; 
}

static const char *magic_gtk_widget(void *ptr) { 
    if (ptr) g_object_unref(ptr);
    return "gtk_widget";
}

static const char *magic_gtk_textbuf(void *ptr) { 
    if (ptr) g_object_unref(ptr);
    return "gtk_text_buffer";
}

static cell *make_special_gtk_app(GtkApplication *gp) {
    return cell_special(magic_gtk_app, (void *)gp);
}

static cell *make_special_gtk_widget(GtkWidget *wp) {
    return cell_special(magic_gtk_widget, (void *)wp);
}

static cell *make_special_gtk_textbuf(GtkTextBuffer *wp) {
    return cell_special(magic_gtk_textbuf, (void *)wp);
}

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
static set cgtk_orientation[] = {
    { "horizontal",        NIL, GTK_ORIENTATION_HORIZONTAL },
    { "vertical",          NIL, GTK_ORIENTATION_VERTICAL },
    { NULL,                NIL, 0 }
};

// enum GtkWindowType
static set cgtk_window_type[] = {
    { "toplevel",          NIL, GTK_WINDOW_TOPLEVEL },
    { "popup",             NIL, GTK_WINDOW_POPUP },
    { NULL,                NIL, 0 }
};

// enum GtkBaselinePosition
static set cgtk_baseline_position[] = {
    { "top",               NIL, GTK_BASELINE_POSITION_TOP },
    { "center",            NIL, GTK_BASELINE_POSITION_CENTER },
    { "bottom",            NIL, GTK_BASELINE_POSITION_BOTTOM },
    { NULL,                NIL, 0 }
};

// enum GtkJustification
static set cgtk_justification[] = {
    { "left",              NIL, GTK_JUSTIFY_LEFT },
    { "right",             NIL, GTK_JUSTIFY_RIGHT },
    { "center",            NIL, GTK_JUSTIFY_CENTER },
    { "fill",              NIL, GTK_JUSTIFY_FILL },
    { NULL,                NIL, 0 }
};

// enum GtkPositionType
static set cgtk_position_type[] = {
    { "left",              NIL, GTK_POS_LEFT },
    { "right",             NIL, GTK_POS_RIGHT },
    { "top",               NIL, GTK_POS_TOP },
    { "bottom",            NIL, GTK_POS_BOTTOM },
    { NULL,                NIL, 0 }
};

// enum GtkReliefStyle
static set cgtk_relief_style[] = {
    { "normal",            NIL, GTK_RELIEF_NORMAL },
    { "half",              NIL, GTK_RELIEF_HALF },
    { "none",              NIL, GTK_RELIEF_NONE },
    { NULL,                NIL, 0 }
};

// enum GtkTextDirection
static set cgtk_text_direction[] = {
    { "none",              NIL, GTK_TEXT_DIR_NONE },
    { "ltr",               NIL, GTK_TEXT_DIR_LTR },
    { "rtl",               NIL, GTK_TEXT_DIR_RTL },
    { NULL,                NIL, 0 }
};

// enum GdkGravity
static set cgdk_gravity[] = {
    { "north_west",        NIL, GDK_GRAVITY_NORTH_WEST },
    { "north",             NIL, GDK_GRAVITY_NORTH },
    { "north_east",        NIL, GDK_GRAVITY_NORTH_EAST },
    { "west",              NIL, GDK_GRAVITY_WEST },
    { "center",            NIL, GDK_GRAVITY_CENTER },
    { "east",              NIL, GDK_GRAVITY_EAST },
    { "south_west",        NIL, GDK_GRAVITY_SOUTH_WEST },
    { "south",             NIL, GDK_GRAVITY_SOUTH },
    { "south_east",        NIL, GDK_GRAVITY_SOUTH_EAST },
    { "static",            NIL, GDK_GRAVITY_STATIC },
    { NULL,                NIL, 0 }
};

// GdkModifierType for keys
// https://developer.gnome.org/gdk3/stable/gdk3-Windows.html#GdkModifierType
static set cgdk_modifier_type[] = {
    { "shift",             NIL, GDK_SHIFT_MASK },
    { "lock",              NIL, GDK_LOCK_MASK },
    { "control",           NIL, GDK_CONTROL_MASK },
    { "mod1",              NIL, GDK_MOD1_MASK },
    { "mod2",              NIL, GDK_MOD2_MASK },
    { "mod3",              NIL, GDK_MOD3_MASK },
    { "mod4",              NIL, GDK_MOD4_MASK },
    { "mod5",              NIL, GDK_MOD5_MASK },
    { "button1",           NIL, GDK_BUTTON1_MASK },
    { "button2",           NIL, GDK_BUTTON2_MASK },
    { "button3",           NIL, GDK_BUTTON3_MASK },
    { "button4",           NIL, GDK_BUTTON4_MASK },
    { "button5",           NIL, GDK_BUTTON5_MASK },
// TODO fill in more
    { NULL,                NIL, 0 }
};

// GdkEventMask
// https://developer.gnome.org/gdk3/stable/gdk3-Events.html#GdkEventMask
static set cgdk_event_mask[] = {
    { "exposure",          NIL, GDK_EXPOSURE_MASK },
    { "pointer_motion",    NIL, GDK_POINTER_MOTION_MASK },
    // deprecated:              GDK_POINTER_MOTION_HINT_MASK
    { "button_motion",     NIL, GDK_BUTTON_MOTION_MASK },
    { "button1_motion",    NIL, GDK_BUTTON1_MOTION_MASK },
    { "button2_motion",    NIL, GDK_BUTTON2_MOTION_MASK },
    { "button3_motion",    NIL, GDK_BUTTON3_MOTION_MASK },
    { "button_press",      NIL, GDK_BUTTON_PRESS_MASK },
    { "button_release",    NIL, GDK_BUTTON_RELEASE_MASK },
    { "key_press",         NIL, GDK_KEY_PRESS_MASK },
    { "key_release",       NIL, GDK_KEY_RELEASE_MASK },
    { "enter_notify",      NIL, GDK_ENTER_NOTIFY_MASK },
    { "leave_notify",      NIL, GDK_LEAVE_NOTIFY_MASK },
    { "focus_change",      NIL, GDK_FOCUS_CHANGE_MASK },
    { "structure",         NIL, GDK_STRUCTURE_MASK },
    { "property_change",   NIL, GDK_PROPERTY_CHANGE_MASK },
    { "visibility_notify", NIL, GDK_VISIBILITY_NOTIFY_MASK },
    { "proximity_in",      NIL, GDK_PROXIMITY_IN_MASK },
    { "proximity_out",     NIL, GDK_PROXIMITY_OUT_MASK },
    { "scroll",            NIL, GDK_SCROLL_MASK },
    { "touch",             NIL, GDK_TOUCH_MASK },
    { "smooth_scroll",     NIL, GDK_SMOOTH_SCROLL_MASK },
    { "touchpad_gesture",  NIL, GDK_TOUCHPAD_GESTURE_MASK },
    { "tablet_pad",        NIL, GDK_TABLET_PAD_MASK },
    { "all_events",        NIL, GDK_ALL_EVENTS_MASK }, // TODO special
    { NULL,                NIL, 0 }
};

// set of signals and types
// 0 is no extra, 1 is integer extra, 2. is event extra
// https://developer.gnome.org/gtk3/stable/GtkWidget.html#GtkWidget-accel-closures-changed
// https://developer.gnome.org/gdk3/unstable/gdk3-Event-Structures.html
static set cgdk_signals[] = {
    { "activate",                   NIL, 0 },
    { "clicked",                    NIL, 1 },
    { "enter",                      NIL, 1 },
    { "leave",                      NIL, 1 },
    { "pressed",                    NIL, 1 },
    { "released",                   NIL, 1 },
    { "destroy",                    NIL, 1 },
    { "hide",                       NIL, 1 },
    { "map",                        NIL, 1 },
    { "unmap",                      NIL, 1 },
    { "grab_focus",                 NIL, 1 },
    { "popup_menu",                 NIL, 1 },
    { "realize",                    NIL, 1 },
    { "unrealize",                  NIL, 1 },
    { "show",                       NIL, 1 },
    { "response",                   NIL, 2 },
    { "event",                      NIL, 3 },
    { "button_press_event",         NIL, 3 },
    { "button_release_event",       NIL, 3 },
    { "scroll_event",               NIL, 3 },
    { "motion_notify_event",        NIL, 3 },
    { "delete_event",               NIL, 3 },
    { "destroy_event",              NIL, 3 },
    { "expose_event",               NIL, 3 },
    { "key_press_event",            NIL, 3 },
    { "key_release_event",          NIL, 3 },
    { "enter_notify_event",         NIL, 3 },
    { "leave_notify_event",         NIL, 3 },
    { "configure_event",            NIL, 3 },
    { "focus_in_event",             NIL, 3 },
    { "focus_out_event",            NIL, 3 },
    { "map_event",                  NIL, 3 },
    { "unmap_event",                NIL, 3 },
    { "property_notify_event",      NIL, 3 },
    { "selection_clear_event",      NIL, 3 },
    { "selection_request_event",    NIL, 3 },
    { "selection_notify_event",     NIL, 3 },
    { "proximity_in_event",         NIL, 3 },
    { "proximity_out_event",        NIL, 3 },
    { "visibility_notify_event",    NIL, 3 },
    { "client_event",               NIL, 3 },
    { "no_expose_event",            NIL, 3 },
    { "window_state_event",         NIL, 3 },
    // TODO boolean
    // "grab_notify"
    // TODO widget
    // "parent_set"
    // TODO gtkdirectiontype
    // "focus"
    // "keynav_failed"
    // "move_focus"
    // TODO gtktestdirection
    // "direction_changed"
    // TODO cairocontext
    // "draw"
    // TODO gparamspec
    // "child-notify"
    // TODO dragcontext
    // "drag_begin"
    // "drag_data_delete"
    // "drag_end"
    // TODO dragcontext, x, y, t
    // "drag_data_get"
    // "drag_drop"
    // TODO dragcontext, seldata, info, t
    // "drag_motion"
    // TODO dragcontext, x, y, seldata, info, t
    // "drag_data_received"
    // TODO and more...
    { NULL,                NIL, 0 }
};

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
    g_object_ref(result); \
    return make_special_gtk_widget(result); \
}
// TODO above will create copies, ideally we should keep track of them in an assoc

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
    return cell_boolean(bval); \
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
    return cell_cstring(cstr); \
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
    g_object_ref(result); \
    return make_special_gtk_widget(result); \
}

#define WIDGET_GET_FROMSET(gname, gtype, gset) \
static cell *cgtk_##gname(cell *widget) { \
    GtkWidget *wp; \
    integer_t sval; \
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_error(); \
    sval = gtk_##gname(gtype(wp)); \
    return cell_fromset(gset, sval); \
}

#define WIDGET_SET_FROMSET(gname, gtype, gset) \
static cell *cgtk_##gname(cell *widget, cell *key) { \
    GtkWidget *wp; \
    integer_t sval = 0; \
    if (!get_gtkwidget(widget, &wp, key) \
     || !get_fromset(gset, key, &sval)) return cell_error(); \
    gtk_##gname(gtype(wp), sval); \
    return cell_void(); \
}

#define WIDGET_INT_GET_FROMSET(gname, gtype, gset) \
static cell *cgtk_##gname(cell *widget, cell *ival) { \
    GtkWidget *wp; \
    integer_t val; \
    integer_t sval; \
    if (!get_gtkwidget(widget, &wp, ival) \
     || !get_integer(ival, &val, NIL)) return cell_error(); \
    sval = gtk_##gname(gtype(wp), val); \
    return cell_fromset(gset, sval); \
}

#define WIDGET_INT_SET_FROMSET(gname, gtype, gset) \
static cell *cgtk_##gname(cell *args) { \
    cell *widget; \
    cell *ival; \
    cell *key; \
    GtkWidget *wp; \
    integer_t val; \
    integer_t sval = 0; \
    if (!arg3(args, &widget, &ival, &key)) { \
        return cell_error(); \
    } \
    if (!get_gtkwidget(widget, &wp, ival)) { \
        cell_unref(key); \
        return cell_error(); \
    } \
    if (!get_integer(ival, &val, key) \
     || !get_fromset(gset, key, &sval)) return cell_error(); \
    gtk_##gname(gtype(wp), val, sval); \
    return cell_void(); \
}

#define WIDGET_GET_MASKSET(gname, gtype, gset) \
static cell *cgtk_##gname(cell *widget) { \
    GtkWidget *wp; \
    integer_t ival; \
    if (!get_gtkwidget(widget, &wp, NIL)) return cell_error(); \
    ival = gtk_##gname(gtype(wp)); \
    return cell_maskset(gset, ival); \
}

#define WIDGET_SET_MASKSET(gname, gtype, gset) \
static cell *cgtk_##gname(cell *widget, cell *keys) { \
    GtkWidget *wp; \
    integer_t ival = 0; \
    if (!get_gtkwidget(widget, &wp, keys) \
     || !get_maskset(gset, keys, &ival)) return cell_error(); \
    gtk_##gname(gtype(wp), ival); \
    return cell_void(); \
}

#define VOID_GET_WIDGET_NEW(gname) \
static cell *cgtk_##gname(cell *args) { \
    GtkWidget *result; \
    arg0(args); \
    result = gtk_##gname(); /* does g_object_ref() */ \
    return make_special_gtk_widget(result); \
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
    return cell_boolean(bval); \
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
    return cell_cstring(cstr); \
}

#define FROMSET_GET_WIDGET_NEW(gname, gset) \
static cell *cgtk_##gname(cell *key) { \
    GtkWidget *result; \
    integer_t ival = 0; \
    if (!get_fromset(gset, key, &ival)) return cell_error(); \
    result = gtk_##gname(ival); /* does g_object_ref() */ \
    return make_special_gtk_widget(result); \
}

#define CSTRING_GET_WIDGET_NEW(gname) \
static cell *cgtk_##gname(cell *str) { \
    GtkWidget *result; \
    char_t *cstr; \
    if (!peek_cstring(str, &cstr, NIL)) return cell_error(); \
    result = gtk_##gname(cstr); /* does g_object_ref() */ \
    cell_unref(str); \
    return make_special_gtk_widget(result); \
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
    gp = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE); // does g_object_ref()

    return make_special_gtk_app(gp);
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
    window = gtk_application_window_new(gp); // does g_object_ref()
    // TODO error check

    return make_special_gtk_widget(window);
}

////////////////////////////////////////////////////////////////
//
//  box
//

FROMSET_GET_WIDGET_NEW(button_box_new, cgtk_orientation)

////////////////////////////////////////////////////////////////
//
//  button
//  https://developer.gnome.org/gtk3/stable/GtkButton.html
//

VOID_GET_WIDGET_NEW(button_new)
CSTRING_GET_WIDGET_NEW(button_new_with_label)
// TODO gtk_button_new_with_mnemonic() see gtk_button_set_use_underline()
// gtk_button_new_from_icon_name (), use gtk_button_new() and gtk_button_set_image().instead
WIDGET_VOID(button_clicked, GTK_BUTTON)
// TODO gtk_button_set_relief(button, reliefstyle)
WIDGET_GET_FROMSET(button_get_relief, GTK_BUTTON, cgtk_relief_style)
WIDGET_SET_FROMSET(button_set_relief, GTK_BUTTON, cgtk_relief_style)
WIDGET_GET_CSTRING(button_get_label, GTK_BUTTON)
WIDGET_SET_CSTRING(button_set_label, GTK_BUTTON)
WIDGET_GET_BOOL(button_get_use_underline, GTK_BUTTON)
WIDGET_SET_BOOL(button_set_use_underline, GTK_BUTTON)
WIDGET_SET_WIDGET(button_set_image, GTK_BUTTON)
WIDGET_GET_WIDGET(button_get_image, GTK_BUTTON)
WIDGET_GET_FROMSET(button_get_image_position, GTK_BUTTON, cgtk_position_type)
WIDGET_SET_FROMSET(button_set_image_position, GTK_BUTTON, cgtk_position_type)
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

VOID_GET_WIDGET_NEW(dialog_new)
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

VOID_GET_WIDGET_NEW(grid_new)

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
WIDGET_INT_GET_FROMSET(grid_get_row_baseline_position, GTK_GRID, cgtk_baseline_position)
WIDGET_INT_SET_FROMSET(grid_set_row_baseline_position, GTK_GRID, cgtk_baseline_position)


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

CSTRING_GET_WIDGET_NEW(image_new_from_file)

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
VOID_GET_WIDGET_NEW(image_new)
WIDGET_SET_INT(image_set_pixel_size, GTK_IMAGE)
WIDGET_GET_INT(image_get_pixel_size, GTK_IMAGE)

////////////////////////////////////////////////////////////////
//
//  label
//  https://developer.gnome.org/gtk3/stable/GtkLabel.html
//
CSTRING_GET_WIDGET_NEW(label_new)

WIDGET_SET_CSTRING(label_set_text, GTK_LABEL)
// TODO    label_set_attributes
WIDGET_SET_CSTRING(label_set_markup, GTK_LABEL) // TODO g_markup_escape_text() or g_markup_printf_escaped():
WIDGET_SET_CSTRING(label_set_markup_with_mnemonic, GTK_LABEL) // TODO ditto
WIDGET_SET_CSTRING(label_set_pattern, GTK_LABEL)
WIDGET_SET_FROMSET(label_set_justify, GTK_LABEL, cgtk_justification)
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
CSTRING_GET_WIDGET_NEW(label_new_with_mnemonic)
WIDGET_SET_INT_INT(label_select_region, GTK_LABEL)
WIDGET_SET_WIDGET(label_set_mnemonic_widget, GTK_LABEL)
WIDGET_SET_BOOL(label_set_selectable, GTK_LABEL)
WIDGET_SET_CSTRING(label_set_text_with_mnemonic, GTK_LABEL)
// TODO gtk_label_get_attributes
WIDGET_GET_FROMSET(label_get_justify, GTK_LABEL, cgtk_justification)
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
    textbuf = gtk_text_buffer_new(NULL); // does g_object_ref()
    return make_special_gtk_textbuf(textbuf);
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
VOID_GET_WIDGET_NEW(text_view_new)

// TODO TEXTBUF_GET_WIDGET_NEW(text_view_new_with_buffer)
static cell *cgtk_text_view_new_with_buffer(cell *tb) {
    GtkWidget *text_view;
    GtkTextBuffer *textbuf;
    if (!get_gtktextbuffer(tb, &textbuf, NIL)) {
        return cell_error();
    }
    text_view = gtk_text_view_new_with_buffer(textbuf); // does g_object_ref()
    return make_special_gtk_widget(text_view);
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
    cell_unref(widget); // TODO may invoke a g_object_unref()
    gtk_widget_destroy(wp);
    return cell_void();
}

WIDGET_GET_BOOL(widget_in_destruction, GTK_WIDGET)
// TODO gtk_widget_destroyed
WIDGET_VOID(widget_show, GTK_WIDGET)
WIDGET_VOID(widget_show_now, GTK_WIDGET)
WIDGET_VOID(widget_hide, GTK_WIDGET)
// TODO gtk_widget_draw  cairo
WIDGET_VOID(widget_queue_draw, GTK_WIDGET)
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
WIDGET_SET_MASKSET(widget_set_events, GTK_WIDGET, cgdk_event_mask)
WIDGET_GET_MASKSET(widget_get_events, GTK_WIDGET, cgdk_event_mask)
WIDGET_SET_MASKSET(widget_add_events, GTK_WIDGET, cgdk_event_mask)
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
WIDGET_SET_FROMSET(widget_set_direction, GTK_WIDGET, cgtk_text_direction)
WIDGET_GET_FROMSET(widget_get_direction, GTK_WIDGET, cgtk_text_direction)
//VOID_SET_FROMSET(widget_set_default_direction, GTK_WIDGET, cgtk_text_direction)
//VOID_GET_FROMSET(widget_get_default_direction, GTK_WIDGET, cgtk_text_direction)
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

FROMSET_GET_WIDGET_NEW(window_new, cgtk_window_type)
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
WIDGET_SET_FROMSET(window_set_gravity, GTK_WINDOW, cgdk_gravity)
WIDGET_GET_FROMSET(window_get_gravity, GTK_WINDOW, cgdk_gravity)
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
WIDGET_GET_FROMSET(window_get_window_type, GTK_WINDOW, cgtk_window_type)
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

// callback, no extra argument provided, but need to wait untill finished
static void cgtk_callback_wait(GtkWidget* gp, gpointer data) {
    // for activate, gp is GtkApplication *
    // ignore gp (for now)
    cell *prog = (cell *)data;

 // assert(cell_is_func((cell *)data));
 // assert(cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_app)
 //     || cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_widget));
 // assert((GtkApplication *) (cell_car(cell_cdr((cell *)data))->_.special.ptr) == gp);
 // printf("\n***callback***\n");

 // TODO seems like this one needs to finish its job before we return
 // interrupt(cell_ref(prog), NIL /*env*/, NIL /*stack*/);
    run_main(cell_ref(prog), NIL /*env*/, NIL /*stack*/, 0);
}

// callback, no extra argument provided
static void cgtk_callback_none(GtkWidget* gp, gpointer data) {
    // for activate, gp is GtkApplication *
    // ignore gp (for now)
    cell *prog = (cell *)data;

//  interrupt(cell_ref(prog), NIL /*env*/, NIL /*stack*/);
    run_main(cell_ref(prog), NIL /*env*/, NIL /*stack*/, 0);
}

// callback, argument provided is an int
static void cgtk_callback_int(GtkWidget* gp, gint extra, gpointer data) {
    // ignore gp (for now)
    cell *prog = (cell *)data;
    cell *stack = cell_list(cell_integer(extra), NIL);

//  interrupt(cell_ref(prog), NIL /*env*/, stack);
    run_main(cell_ref(prog), NIL /*env*/, stack, 0);
}

// callback, argument provided is a GdkEvent
static void cgtk_callback_event(GtkWidget* gp, GdkEvent *extra, gpointer data) {
    // ignore gp (for now)
    cell *prog = (cell *)data;
    cell *stack = cell_list(cell_gdkevent(extra), NIL);

//  interrupt(cell_ref(prog), NIL /*env*/, stack);
    run_main(cell_ref(prog), NIL /*env*/, stack, 0);
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
    // match any type of special first
    // TODO may be better to test the two at this point
    if (!peek_special(NULL, app, (void **)&gp, callback)) {
        cell_unref(hook); // TODO
        return cell_error();
    }
    if (!peek_symbol(hook, &signal, callback)) {
        cell_unref(app);
        return cell_error();
    }
    if (app->_.special.magicf != magic_gtk_app
     && app->_.special.magicf != magic_gtk_widget) {
        cell_unref(error_rt1("not a widget nor application", cell_ref(app)));
        cell_unref(app);
        cell_unref(hook);
        return cell_error();
    }
    cell *callbackprog = NIL;
    integer_t type;

    if (!get_fromset(cgdk_signals, hook, &type)) {
        cell_unref(app);
        return cell_error();
    }
    // TODO hook should be retained to know string??

    switch (type) {
    case 0: // no extra data, needs to complete before we return
        callbackprog = compile_thunk(callback, 0);

        // TODO gulong tag =
        g_signal_connect(gp, signal, G_CALLBACK(cgtk_callback_wait), callbackprog);
        break;

    case 1: // no extra data provided
        callbackprog = compile_thunk(callback, 0);

        g_signal_connect(gp, signal, G_CALLBACK(cgtk_callback_none), callbackprog);
        break;

    case 2: // extra parameter: integer response_id
        callbackprog = compile_thunk(callback, 1);

        g_signal_connect(gp, signal, G_CALLBACK(cgtk_callback_int), callbackprog);
        break;

    case 3: // extra parameter: GdkEvent *event
        callbackprog = compile_thunk(callback, 1);

        // https://developer.gnome.org/gdk3/unstable/gdk3-Event-Structures.html
        g_signal_connect(gp, signal, G_CALLBACK(cgtk_callback_event), callbackprog);
        break;

    default:
        assert(0);
    }

    // cell_unref(callbackprog); // TODO when to unref callbackprog ???
    // TODO void g_signal_handler_disconnect( gp, tag );

    cell_unref(app);
    return cell_void();
}

////////////////////////////////////////////////////////////////
//
//  GdkEvent
//  https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html
//

static cell *cell_gdkevent(GdkEvent *event) {
    cell *a = cell_assoc();
    cell *type = cell_symbol("type");
    // TODO ignore type->send_event
    assert(event);
#if 0
    if (((GdkEventAny *)event)->window) {
        g_object_ref(((GdkEventAny *)event)->window);
        assoc_set(a, cell_symbol("window"), make_special_gdk_window(((GdkEventAny *)event)->window));
    }
#endif
    switch (event->type) {
    case GDK_NOTHING: // a special code to indicate a null event.
      nullevent:
        assoc_set(a, type, cell_symbol("null"));
        break;

    case GDK_DELETE: // the window manager has requested that the toplevel window be hidden or destroyed, usually when the user clicks on a special icon in the title bar.
        assoc_set(a, type, cell_symbol("delete"));
        break;

    case GDK_DESTROY: // the window has been destroyed.
        assoc_set(a, type, cell_symbol("destroy"));
        break;

    case GDK_EXPOSE: // all or part of the window has become visible and needs to be redrawn.
        assoc_set(a, type, cell_symbol("expose"));
        break;

    case GDK_MOTION_NOTIFY: // the pointer (usually a mouse) has moved.
        assoc_set(a, type, cell_symbol("motion_notify"));
        {
            // https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html#GdkEventMotion
            GdkEventMotion *em = (GdkEventMotion *)event;
            // TODO device
            assoc_set(a, cell_symbol("time"), cell_real(em->time/1000.0)); // TODO see also #use("time"), coordinate
            assoc_set(a, cell_symbol("x"), cell_real(em->x));
            assoc_set(a, cell_symbol("y"), cell_real(em->y));
            assoc_set(a, cell_symbol("state"), cell_integer(em->state)); // TODO https://developer.gnome.org/gdk3/stable/gdk3-Windows.html#GdkModifierType
            assoc_set(a, cell_symbol("is_hint"), cell_boolean(em->is_hint));
            assoc_set(a, cell_symbol("x_root"), cell_real(em->x_root));
            assoc_set(a, cell_symbol("y_root"), cell_real(em->y_root));
            if (em->axes && isfinite(em->axes[0])) {
                assoc_set(a, cell_symbol("x_axes"), cell_real(em->axes[0]));
                assoc_set(a, cell_symbol("y_axes"), cell_real(em->axes[1]));
            }
        }
        break;

    case GDK_BUTTON_PRESS: // a mouse button has been pressed.
        assoc_set(a, type, cell_symbol("button_press"));
      buttonevent:
        {
            // https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html#GdkEventButton
            // TODO device
            GdkEventButton *eb = (GdkEventButton *)event;
            assoc_set(a, cell_symbol("time"), cell_real(eb->time/1000.0));
            assoc_set(a, cell_symbol("x"), cell_real(eb->x));
            assoc_set(a, cell_symbol("y"), cell_real(eb->y));
            assoc_set(a, cell_symbol("state"), cell_integer(eb->state)); // TODO https://developer.gnome.org/gdk3/stable/gdk3-Windows.html#GdkModifierType
            assoc_set(a, cell_symbol("button"), cell_integer(eb->button)); // usually 1 to 5
            assoc_set(a, cell_symbol("x_root"), cell_real(eb->x_root));
            assoc_set(a, cell_symbol("y_root"), cell_real(eb->y_root));
            if (eb->axes && isfinite(eb->axes[0])) {
                assoc_set(a, cell_symbol("x_axes"), cell_real(eb->axes[0]));
                assoc_set(a, cell_symbol("y_axes"), cell_real(eb->axes[1]));
            }
        }
        break;

    case GDK_DOUBLE_BUTTON_PRESS: // a mouse button has been double-clicked (clicked twice within a short period of time). Note that each click also generates a GDK_BUTTON_PRESS event.
    // alias: GDK_2BUTTON_PRESS
        assoc_set(a, type, cell_symbol("double_button_press"));
        goto buttonevent;

    case GDK_TRIPLE_BUTTON_PRESS: // a mouse button has been clicked 3 times in a short period of time. Note that each click also generates a GDK_BUTTON_PRESS event.
    // alias: GDK_3BUTTON_PRESS
        assoc_set(a, type, cell_symbol("triple_button_press"));
        goto buttonevent;

    case GDK_BUTTON_RELEASE: // a mouse button has been released.
        assoc_set(a, type, cell_symbol("button_release"));
        goto buttonevent;

    case GDK_KEY_PRESS: // a key has been pressed.
        assoc_set(a, type, cell_symbol("key_press"));
      keyevent:
        {
            // https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html#GdkEventKey
            GdkEventKey *ek = (GdkEventKey *)event;
            assoc_set(a, cell_symbol("time"), cell_real(ek->time/1000.0));
            assoc_set(a, cell_symbol("keyval"), cell_integer(ek->keyval));
            assoc_set(a, cell_symbol("state"), cell_maskset(cgdk_modifier_type, ek->state));

            // TODO depreceated, use gdk_unicode_to_keyval() instead to get UTF8
            assoc_set(a, cell_symbol("string"), cell_nastring(ek->string, ek->length));

            assoc_set(a, cell_symbol("group"), cell_integer(ek->group));
            assoc_set(a, cell_symbol("hardware_keycode"), cell_integer(ek->hardware_keycode));
            assoc_set(a, cell_symbol("is_modifier"), cell_boolean(ek->is_modifier));
        }
        break;

    case GDK_KEY_RELEASE: // a key has been released.
        assoc_set(a, type, cell_symbol("key_release"));
        goto keyevent;

    case GDK_ENTER_NOTIFY: // the pointer has entered the window.
        assoc_set(a, type, cell_symbol("enter_notify"));
        break;

    case GDK_LEAVE_NOTIFY: // the pointer has left the window.
        assoc_set(a, type, cell_symbol("leave_notify"));
        break;

    case GDK_FOCUS_CHANGE: // the keyboard focus has entered or left the window.
        assoc_set(a, type, cell_symbol("focus_change"));
        {
            // https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html#GdkEventFocus
            GdkEventFocus *ef = (GdkEventFocus *)event;
            assoc_set(a, cell_symbol("in"), cell_boolean(ef->in));
        }
        break;

    case GDK_CONFIGURE: // the size, position or stacking order of the window has changed. Note that GTK+ discards these events for GDK_WINDOW_CHILD windows.
        assoc_set(a, type, cell_symbol("configure"));
        {
            // https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html#GdkEventConfigure
            GdkEventConfigure *ec = (GdkEventConfigure *)event;
            assoc_set(a, cell_symbol("x"),      cell_integer(ec->x));
            assoc_set(a, cell_symbol("y"),      cell_integer(ec->y));
            assoc_set(a, cell_symbol("width"),  cell_integer(ec->width));
            assoc_set(a, cell_symbol("height"), cell_integer(ec->height));
        }
        break;

    case GDK_MAP: // the window has been mapped.
        assoc_set(a, type, cell_symbol("map"));
        break;

    case GDK_UNMAP: // the window has been unmapped.
        assoc_set(a, type, cell_symbol("unmap"));
        break;

    case GDK_PROPERTY_NOTIFY: // a property on the window has been changed or deleted.
        assoc_set(a, type, cell_symbol("property_notify"));
        break;

    case GDK_SELECTION_CLEAR: // the application has lost ownership of a selection.
        assoc_set(a, type, cell_symbol("selection_clear"));
        break;

    case GDK_SELECTION_REQUEST: // another application has requested a selection.
        assoc_set(a, type, cell_symbol("selection_request"));
        break;

    case GDK_SELECTION_NOTIFY: // a selection has been received.
        assoc_set(a, type, cell_symbol("selection_notify"));
        break;

    case GDK_PROXIMITY_IN: // an input device has moved into contact with a sensing surface (e.g. a touchscreen or graphics tablet).
        assoc_set(a, type, cell_symbol("proximity_in"));
        break;

    case GDK_PROXIMITY_OUT: // an input device has moved out of contact with a sensing surface.
        assoc_set(a, type, cell_symbol("proximity_out"));
        break;

    case GDK_DRAG_ENTER: // the mouse has entered the window while a drag is in progress.
        assoc_set(a, type, cell_symbol("drag_enter"));
        break;

    case GDK_DRAG_LEAVE: // the mouse has left the window while a drag is in progress.
        assoc_set(a, type, cell_symbol("drag_leave"));
        break;

    case GDK_DRAG_MOTION: // the mouse has moved in the window while a drag is in progress.
        assoc_set(a, type, cell_symbol("drag_motion"));
        break;

    case GDK_DRAG_STATUS: // the status of the drag operation initiated by the window has changed.
        assoc_set(a, type, cell_symbol("drag_status"));
        break;

    case GDK_DROP_START: // a drop operation onto the window has started.
        assoc_set(a, type, cell_symbol("drop_start"));
        break;

    case GDK_DROP_FINISHED: // the drop operation initiated by the window has completed.
        assoc_set(a, type, cell_symbol("drop_finished"));
        break;

    case GDK_CLIENT_EVENT: // a message has been received from another application.
        assoc_set(a, type, cell_symbol("client_event"));
        break;

    case GDK_VISIBILITY_NOTIFY: // the window visibility status has changed.
        assoc_set(a, type, cell_symbol("visibility_notify"));
        // NOTE depreceated
        break;

    case GDK_SCROLL: // the scroll wheel was turned
        assoc_set(a, type, cell_symbol("scroll"));
        break;

    case GDK_WINDOW_STATE: // the state of a window has changed. See GdkWindowState for the possible window states
        assoc_set(a, type, cell_symbol("window_state"));
        break;

    case GDK_SETTING: // a setting has been modified.
        assoc_set(a, type, cell_symbol("setting"));
        break;

    case GDK_OWNER_CHANGE: // the owner of a selection has changed. This event type was added in 2.6
        assoc_set(a, type, cell_symbol("owner_change"));
        break;

    case GDK_GRAB_BROKEN: // a pointer or keyboard grab was broken.
        assoc_set(a, type, cell_symbol("grab_broken"));
        break;

    case GDK_DAMAGE: // the content of the window has been changed.
        assoc_set(a, type, cell_symbol("damage"));
        break;

    case GDK_TOUCH_BEGIN: // A new touch event sequence has just started.
        assoc_set(a, type, cell_symbol("touch_begin"));
      touchevent:
        {
            GdkEventTouch *et = (GdkEventTouch *)event;
            // https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html#GdkEventTouch
            // TODO sequence, emulation_pointer, device
            assoc_set(a, cell_symbol("time"), cell_real(et->time/1000.0));
            assoc_set(a, cell_symbol("x"), cell_real(et->x)); 
            assoc_set(a, cell_symbol("y"), cell_real(et->y));
            assoc_set(a, cell_symbol("x_root"), cell_real(et->x_root));
            assoc_set(a, cell_symbol("y_root"), cell_real(et->y_root));
            if (et->axes && isfinite(et->axes[0])) {
                assoc_set(a, cell_symbol("x_axes"), cell_real(et->axes[0]));
                assoc_set(a, cell_symbol("y_axes"), cell_real(et->axes[1]));
            }
        }
        break;

    case GDK_TOUCH_UPDATE: // A touch event sequence has been updated.
        assoc_set(a, type, cell_symbol("touch_update"));
        goto touchevent;

    case GDK_TOUCH_END: // A touch event sequence has finished.
        assoc_set(a, type, cell_symbol("touch_end"));
        goto touchevent;

    case GDK_TOUCH_CANCEL: // A touch event sequence has been canceled.
        assoc_set(a, type, cell_symbol("touch_cancel"));
        goto touchevent;

    case GDK_TOUCHPAD_SWIPE: // A touchpad swipe gesture event, the current state is determined by its phase field.
        assoc_set(a, type, cell_symbol("touchpad_swipe"));
        goto touchevent;

    case GDK_TOUCHPAD_PINCH: // A touchpad pinch gesture event, the current state is determined by its phase field.
        assoc_set(a, type, cell_symbol("touchpad_pinch"));
        goto touchevent;

    case GDK_PAD_BUTTON_PRESS: // A tablet pad button press event.
        assoc_set(a, type, cell_symbol("pad_button_press"));
        break;

    case GDK_PAD_BUTTON_RELEASE: // A tablet pad button release event.
        assoc_set(a, type, cell_symbol("pad_button_release"));
        break;

    case GDK_PAD_RING: // A tablet pad axis event from a "ring".
        assoc_set(a, type, cell_symbol("pad_ring"));
        break;

    case GDK_PAD_STRIP: // A tablet pad axis event from a "strip".
        assoc_set(a, type, cell_symbol("pad_strip"));
        break;

    case GDK_PAD_GROUP_MODE: // A tablet pad group mode change.
        assoc_set(a, type, cell_symbol("pad_group_mode"));
        break;

    case GDK_EVENT_LAST: // marks the end of the GdkEventType enumeration
        goto nullevent;
    }
    return a;
}

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
    DEFINE_CFUN1(button_new_with_label)
    DEFINE_CFUN1(button_box_new)
    DEFINE_CFUN1(button_clicked)
    DEFINE_CFUN1(button_get_label)
    DEFINE_CFUN2(button_set_label)
    DEFINE_CFUN1(button_get_relief)
    DEFINE_CFUN2(button_set_relief)
    DEFINE_CFUN1(button_get_use_underline)
    DEFINE_CFUN2(button_set_use_underline)
    DEFINE_CFUN2(button_set_image)
    DEFINE_CFUN1(button_get_image)
    DEFINE_CFUN2(button_set_image_position)
    DEFINE_CFUN1(button_get_image_position)
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
    DEFINE_CFUNN(grid_set_row_baseline_position)
    DEFINE_CFUN2(grid_get_row_baseline_position)
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
    DEFINE_CFUN2(label_set_justify)
    DEFINE_CFUN2(label_set_xalign)
    DEFINE_CFUN2(label_set_yalign)
    DEFINE_CFUN2(label_set_width_chars)
    DEFINE_CFUN2(label_set_max_width_chars)
    DEFINE_CFUN2(label_set_line_wrap)
    DEFINE_CFUN2(label_set_lines)
    DEFINE_CFUN1(label_get_mnemonic_keyval)
    DEFINE_CFUN1(label_get_selectable)
    DEFINE_CFUN1(label_get_text)
    DEFINE_CFUN1(label_new_with_mnemonic)
    DEFINE_CFUNN(label_select_region)
    DEFINE_CFUN2(label_set_mnemonic_widget)
    DEFINE_CFUN2(label_set_selectable)
    DEFINE_CFUN2(label_set_text_with_mnemonic)
    DEFINE_CFUN1(label_get_justify)
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
    DEFINE_CFUN1(text_view_new_with_buffer)
    DEFINE_CFUN1(widget_destroy)
    DEFINE_CFUN1(widget_in_destruction)
    DEFINE_CFUN1(widget_show)
    DEFINE_CFUN1(widget_show_now)
    DEFINE_CFUN1(widget_hide)
    DEFINE_CFUN1(widget_show_all)
    DEFINE_CFUNN(widget_queue_draw)
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
    DEFINE_CFUN2(widget_set_direction)
    DEFINE_CFUN1(widget_get_direction)
    // DEFINE_CFUN1(widget_set_default_direction)
    // DEFINE_CFUNN(widget_get_default_direction)
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
    DEFINE_CFUN2(window_set_gravity)
    DEFINE_CFUN1(window_get_gravity)
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
    DEFINE_CFUN1(window_get_window_type)
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

    return a;
}

