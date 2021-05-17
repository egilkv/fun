// TAB P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

gtk = #use("gtk");

// Alt1: Mutable object oriented style
act = (app){

    // TODO is this parenthesis reqd
    (#use("io")).println("****callback****");

    window = gtk.application_window_new(app);
    gtk.window_set_title(window, "Window");
    gtk.window_set_default_size(window, 200, 200);
    gtk.widget_show_all(window)
};
app = gtk.application_new("org.gtk.example");
gtk.signal_connect(app, 'activate, act);
gtk.application_run(app, []);

/*

// Alt2: Provide assoc as argument; >1 of anyhing needs a list, sequence?
gtk.application_run(
    gtk.application("org.gtk.example").connect('activate, (){
	window = gtk.application_window({
	    title: "Window",
	    default_size: ( 200 : 200),
	    show: #t
	})
    }),
    []
);


// Alt3: Named args. Neat stuff, may add to language. >1 of anything needs a list, sequence?
gtk.application_run(
    gtk.application("org.gtk.example").connect('activate, (){
	window = gtk.application_window(
	    title: "Window",
	    default_size: ( 200 : 200),
	    show: #t
	)
    }),
    []
);


// Alt4: Make functions return their (updated) assoc
gtk.application_run(
    gtk.application("org.gtk.example").connect('activate, (){
	window = gtk.application_window()
	    .title("Window")
	    .default_size(200,200)
	    .show()
    }),
    []
);


// Alt5: provide list of pairs as argumentww
gtk.application_run(
    gtk.application("org.gtk.example").connect('activate, (){
	window = gtk.application_window([
	    'title: "Window",
	    'default_size: ( 200 : 200)
	    'show: #t
	])
    }),
    []
);

*/
