// TAB P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

gtk = #use("gtk3");

app = gtk.application_new("org.gtk.example");

gtk.signal_connect(app, 'activate, (){
    gtk.print("***callback***\n");
    window = gtk.application_window_new(app);
    gtk.window_set_title(window, "Window");
    gtk.window_set_default_size(window, 200, 200);

    // debug:
    gtk.println("window type is ", gtk.window_get_window_type(window) );

    button_box = gtk.button_box_new('horizontal);
    gtk.container_add(window, button_box);

    button = gtk.button_new_with_label("Hello World");
    gtk.signal_connect(button, 'clicked, (){ gtk.print("Hello World\n") });
    // TODO needs continuation
    // gtk.signal_connect(button, 'clicked, (){ gtk.widget_destroy(window) });

    gtk.container_add(button_box, button);

    gtk.widget_show_all(window)
});

gtk.application_run(app, #args);

