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
// static const char *magic_gtk_wid = "gtk_widget";

// TODO do we need a struct??
struct gtk_s {
    GtkApplication *g_app;
};

// a in always unreffed
// dump is unreffed only if error
static int get_special(cell *a, const char *magic, char **valuep, cell *dump) {
    if (!cell_is_special(a, magic)) {
        cell_unref(dump);
        cell_unref(error_rt1("not an appropriate argument", a));
        return 0;
    }
    *valuep = a->_.special.ptr;
    cell_unref(a);
    return 1;
}

static int get_gtk_s(cell *app, struct gtk_s **gpp, cell *dump) {
    return get_special(app, magic_gtk_app, (char **)gpp, dump);
}

static cell *cgtk_application_new(cell *args) {
    cell *aname = NIL;
    cell *flags = NIL;
    cell *app;
    GtkApplication *g_app;

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
    g_app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);

    app = cell_special(sizeof(struct gtk_s), magic_gtk_app);
    ((struct gtk_s *)(app->_.special.ptr))->g_app = g_app;

    return app;
}

static cell *cgtk_application_run(cell *app, cell *arglist) {
    // int status;
    struct gtk_s *gp;
    if (!get_gtk_s(app, &gp, arglist)) {
        return cell_ref(hash_void); // error
    }
    // TODO convert to argc, argv
    // status =
    g_application_run (G_APPLICATION(gp->g_app), 0, NULL);
    // TODO look at status
    cell_unref(arglist);
    return app;
}

static cell *cgtk_application_window_new(cell *app) {
    // GtkWidget *window;
    struct gtk_s *gp;
    if (!get_gtk_s(app, &gp, NIL)) {
        return cell_ref(hash_void); // error
    }
    // window =
    gtk_application_window_new(gp->g_app);
    // TODO make window
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

static void do_callback(GtkApplication* g_app, gpointer data) {
    // struct gtk_s *gp;
    assert(cell_is_list((cell *)data));
    assert(cell_is_special(cell_car(cell_cdr((cell *)data)), magic_gtk_app));
 // assert(((struct gtk_s *)(data->_.cons.car->_.special.ptr))->g_app == g_app);
    printf("\n***callback***\n");

    // TODO should be continuation
    eval(cell_ref((cell *)data), NULL);
}

static cell *cgtk_signal_connect(cell *app, cell *hook, cell *callback) {
    struct gtk_s *gp;
    cell_unref(hook); // TODO
    if (!get_gtk_s(app, &gp, callback)) {
        return cell_ref(hash_void); // error
    }
    // TODO should be continuation instead
    g_signal_connect(gp->g_app, "activate", G_CALLBACK(do_callback), 
                                    cell_list(callback, cell_list(app, NIL)));
    // cell_unref(callback); // TODO when to unref callback ???
    return NIL; // TODO
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
    return assoc;
}

