// TAB P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

gtk = #use("gtk3");

app = gtk.application_new("org.gtk.example");

gtk_application_window = (app, ...) {
    win = gtk.application_window_new(app);
    args = (a) {
	a == [] ? win : { 
	    title:          (){ gtk.window_set_title(win, a[0][1]); args(a[1..]) },
	    border_width:   (){ gtk.container_set_border_width(win, a[0][1]); args(a[1..]) },
	    add:            (){ gtk.container_add(win, a[0][1]); args(a[1..]) }
	}[a[0][0]]()
    };
    args(...)
};

gtk_button = (label, ...) {
    widg = gtk.button_new(label);
    args = (a) {
	a == [] ? widg : {
	    signal_connect: (){ gtk.signal_connect(widg, a[0][1], a[1]); args(a[2..]) }
	}[a[0][0]]()
    };
    args(...)
};

gtk_grid = (...) {
    grid = gtk.grid_new();
    args = (a) {
	a == [] ? grid : {
	    attach:         (){ gtk.grid_attach(grid, a[0][1], a[1]); args(a[2..]) }
	}[a[0][0]]()
    };
    args(...)
};

gtk.signal_connect(app, 'activate, (a){
    window = #undefined;

    button1 = gtk_button("Button 1");
    gtk.signal_connect(button1, 'clicked, (w){ gtk.print("Hello World 1\n") });

    button2 = gtk_button("Button 2");
    gtk.signal_connect(button2, 'clicked, (w){ gtk.print("Hello World 2\n") });

    button = gtk_button("Quit");
    gtk.signal_connect(button, 'clicked, (w){ gtk.widget_destroy(window) });

    grid = gtk_grid();

    window = gtk_application_window(app,
	title:        "Mywindow",
	border_width: 10,
	add:          grid
    );

    gtk.grid_attach(grid, button1, [0, 0, 1, 1]);

    gtk.grid_attach(grid, button2, [1, 0, 1, 1]);

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
