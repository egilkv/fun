// TAB-P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

#include("gtk3.fun");

app = gtk_application("org.gtk.example",
    signal_connect:         'activate, (){

    window = #undefined;

    button1 = gtk_button(
        label:              "Button",
        signal_connect:     'clicked, (){ gtk.println("Hello World") }
    );

    button2 = gtk_button(
        label:              "Dialog",
        signal_connect:     'clicked, (){ popup_dialog() }
    );

    button = gtk_button(
        label:              "Quit",
        signal_connect:     'clicked, (){ gtk.widget_destroy(window) }
    );

    label1 = gtk_label("The Quick Brown Fox Jumps Over a Lazy Dog"
    );

    grid = gtk_grid(
        attach:             [0, 0, 1, 1], button1,
        attach:             [1, 0, 1, 1], button2,
        attach:             [0, 1, 2, 1], button,
        attach:             [0, 2, 2, 1], gtk.image_new_from_file("tux256.png"),
        attach:             [0, 3, 2, 1], label1
    );

    window = gtk_application_window(app,
	title:        "Mywindow",
	border_width: 10,
        add:          grid,
        show_all:     #t
    )
});

popup_dialog = (){

    // TODO parent where/how
    // TODO where is response

    dialog = gtk.dialog_new();
    w1 = gtk.dialog_add_button(dialog, "Option 1", 1);
    w2 = gtk.dialog_add_button(dialog, "Option 2", 2);
    w3 = gtk.dialog_add_button(dialog, "Option 3", 3);

    // destroy when one choice has been made
    gtk.signal_connect(dialog, 'response, (r){ gtk.println("response is ", r); gtk.widget_destroy(dialog) });

    gtk.widget_show_all(dialog)
};


gtk.application_run(app, #args);

