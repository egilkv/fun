// TAB P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

gtk = #use("gtk");

app = gtk.application_new("org.gtk.example");

gtk.signal_connect(app, 'activate, (a){
    gtk.print("***callback***\n");
    window = gtk.application_window_new(a);
    gtk.window_set_title(window, "Window");
    gtk.window_set_default_size(window, 200, 200);

    button_box = gtk.button_box_new('horizontal);
    gtk.container_add(window, button_box);

    button = gtk.button_new("Hello World");
    // TODO signal

    gtk.container_add(button_box, button);

    gtk.widget_show_all(window)
});

gtk.application_run(app, #args);

