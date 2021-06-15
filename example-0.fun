// TAB P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

gtk = #use("gtk3");

/*
app = gtk.application_new("org.gtk.example");

gtk.signal_connect(app, 'activate, () {
    gtk.print("***callback***\n");
    window = gtk.application_window_new(app);
    gtk.window_set_title(window, "Window");
    gtk.window_set_default_size(window, 200, 200);
    gtk.widget_show_all(window)
});
*/
app = gtk.application_new("org.gtk.example"
    connect:    ['activate, (){
	gtk.print("***callback***\n");
	window = gtk.application_window(app,
		 title:         "Window",
		 dfl_size:      [200, 200],
		 show:          #t);
    }]
);
/*
    window = app.window(
		 title:         "Window",
		 dfl_size:      [200, 200],
		 show:          #t);
*/

gtk.application_run(app, #args);

