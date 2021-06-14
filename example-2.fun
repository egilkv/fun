// TAB P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

io = #use("io");
gtk = #use("gtk3");

app = gtk.application_new("org.gtk.example");

gtk_application_window = (app) {
    win = gtk.application_window_new(app);
//  io.writeln("win=", win);
    args = (){ 
//    io.writeln("win2=", win);
      win };
    args();
    win
};

gtk.signal_connect(app, 'activate, (a){
    io.writeln("activate");
//  window = gtk.application_window_new(app);
//  gtk.window_set_title(window, "Window");
//  gtk.container_set_border_width(window, 10);
    window = gtk_application_window(app);
    io.writeln("window=", window);
    gtk.window_set_title(window, "Window");
    gtk.container_set_border_width(window, 10);

    grid = gtk.grid_new();
    gtk.container_add(window, grid);

    button1 = gtk.button_new("Button 1");
    gtk.signal_connect(button1, 'clicked, (w){ gtk.print("Hello World 1\n") });
    gtk.grid_attach(grid, button1, [0, 0, 1, 1]);

    button2 = gtk.button_new("Button 2");
    gtk.signal_connect(button2, 'clicked, (w){ gtk.print("Hello World 2\n") });
    gtk.grid_attach(grid, button2, [1, 0, 1, 1]);

    button = gtk.button_new("Quit");
    // TODO needs continuation
    gtk.signal_connect(button, 'clicked, (w){ gtk.widget_destroy(window) });
    gtk.grid_attach(grid, button, [0, 1, 2, 1]);

    gtk.widget_show_all(window)
});

/*
app = gtk.application_new("org.gtk.example",
    connect:    ['activate, (a){
	window = gtk.application_window(a,
	    title:      "Window",
	    border_width: 10,
	    add:        gtk.grid_new(
			    attach:     [0, 0, 1, 1,
				gtk.button_new("Button 1",
				    connect: ['clicked, (w){ gtk.print("Hello World 1\n") }]
				)
			    ],
			    attach:     [1, 0, 1, 1,
				gtk.button_new("Button 1",
				    connect: ['clicked, (w){ gtk.print("Hello World 2\n") }]
				)
			    ],
			    attach:     [0, 1, 2, 1,
				gtk.button_new("Quit",
				    connect: ['clicked, (w){ gtk.print("Hello World\n") }]
				)
			    ]
			),
	    show:       #t);
	}]
);
*/

gtk.application_run(app, #args);
