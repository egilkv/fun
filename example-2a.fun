// TAB-P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

#include("gtk3.fun");

app = gtk_application("org.gtk.example",
    signal_connect:         'activate, (a){

    window = #undefined;

    button1 = gtk_button(
        label:              "Button 1",
        signal_connect:     'clicked, (w){ gtk.println("Hello World 1") }
    );

    button2 = gtk_button(
        label:              "Button 2",
        signal_connect:     'clicked, (w){ gtk.println("Hello World 2") }
    );

    button = gtk_button(
        label:              "Quit",
        signal_connect:     'clicked, (w){ gtk.widget_destroy(window) }
    );

    grid = gtk_grid(
        attach:             button1, [0, 0, 1, 1],
        attach:             button2, [1, 0, 1, 1],
        attach:             button,  [0, 1, 2, 1]
    );

    window = gtk_application_window(app,
	title:        "Mywindow",
	border_width: 10,
        add:          grid,
        show_all:     #t
    )
});

gtk.application_run(app, #args);
