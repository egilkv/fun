// TAB P
// https://developer.gnome.org/gtk3/stable/gtk-getting-started.html

gtk = #use("gtk3");

app = gtk.application_new("org.gtk.example");

gtk_application_window = (app, ...) {
    win = gtk.application_window_new(app);
    args = (a) {
	a == []
	    ? win
	    : #type(a[0]) != 'label
		? #error("attributes must be attr: val", a[0])
		: a[0][0] == 'title
		    ? [ gtk.window_set_title(win, a[0][1]), args(a[1..]) ]
		    : a[0][0] == 'border_width
			? [ gtk.container_set_border_width(win, a[0][1]), args(a[1..]) ]
			: a[0][0] == 'add
			    ? [ gtk.container_add(win, a[0][1]), args(a[1..]) ]
			    : #error("unknown attr", a[0])
    };
    args(...);
    win
};

gtk.signal_connect(app, 'activate, (a){
    grid = gtk.grid_new();
    gtk.container_add(window, grid);

    window = gtk_application_window(app,
	title:        "Mywindow",
	border_width: 10,
	add:          grid
    );

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
