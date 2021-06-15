// TAB P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

gtk = #use("gtk3");

app = gtk.application_new("org.gtk.example");

/*
gtk.signal_connect(app, 'activate, (){
    gtk.print("***callback***\n");
    window = gtk.application_window_new(a);
    gtk.window_set_title(window, "Window");
    gtk.window_set_default_size(window, 200, 200);

    button_box = gtk.button_box_new('horizontal);
    gtk.container_add(window, button_box);

    button = gtk.button_new("Hello World");
    gtk.signal_connect(button, 'clicked, (){ gtk.print("Hello World\n") });
    // TODO needs continuation
    // gtk.signal_connect(button, 'clicked, (){ gtk.widget_destroy(window) });

    gtk.container_add(button_box, button);

    gtk.widget_show_all(window)
});
*/

app = gtk.application_new("org.gtk.example",
    connect:    ['activate, (){
        gtk.print("***callback***\n");
        window = gtk.application_window(app,
            title:      "Window",
            dfl_size:   [200, 200],
            add:        gtk.button_box_new('horizontal,
                            add:        gtk.button_new("Hello World"),
			    connect:    ['clicked, (){ gtk.print("Hello World\n") }],
			    connect:    ['clicked, (){ gtk.widget_destroy(window) }]
                        ),
            show:       #t);
        }]
);

gtk.application_run(app, #args);

