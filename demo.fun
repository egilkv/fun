// TAB-P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

io = #use("io");
time = #use("time");
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

    label1 = gtk_label("The Quick Brown Fox Jumps Over a Lazy Dog");

    image1 = gtk.image_new_from_file("tux256.png");

    grid = gtk_grid(
        attach: button1,    [0, 0, 1, 1],
        attach: button2,    [1, 0, 1, 1],
        attach: button,     [0, 1, 2, 1],
        attach: image1,     [0, 2, 2, 1],
        attach: label1,     [0, 3, 2, 1]
    );

    window = gtk_application_window(app,
	title:        "Mywindow",
	border_width: 10,
        add:          grid,
        show_all:     #t
    );

    demo_events(window)
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

demo_events = (w) {
    gtk.widget_add_events(w,      gtk.exposure_mask +
                                  gtk.leave_notify_mask +
                                  gtk.button_press_mask +
                                  gtk.key_press_mask +
                                  gtk.key_release_mask +
                                  gtk.pointer_motion_mask +
                                  gtk.pointer_motion_hint_mask);
    gtk.println("events are ", gtk.widget_get_events(w));
//  gtk.signal_connect(w,      'expose_event,        (e){ io.println("Expose:", e) });
    gtk.signal_connect(w,      'configure_event,     (e){ io.println("Configure:", e) });
    gtk.signal_connect(w,      'motion_notify_event, (e){ io.println("Motion:", e) });
    gtk.signal_connect(w,      'key_press_event,     (e){ io.println("Key pressed (t=",time.seconds(),"):", e) });
    gtk.signal_connect(w,      'key_release_event,   (e){ io.println("Key released:", e) });
    gtk.signal_connect(w,      'button_press_event,  (e){ io.println("Button pressed:", e) })
};

gtk.application_run(app, #args);

