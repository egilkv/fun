// TAB-P

gtk = #use("gtk3");

gtk_application = (uri, ...) {
    app = gtk.application_new(uri);
    args = (a) {
        a == [] ? app : {
            signal_connect: (){ gtk.signal_connect(app, a[0][1], a[1]); args(a[2..]) }
	}[a[0][0]]()
    };
    args(...)
};

gtk_application_window = (app, ...) {
    win = gtk.application_window_new(app);
    args = (a) {
	a == [] ? win : { 
	    title:          (){ gtk.window_set_title(win, a[0][1]); args(a[1..]) },
	    border_width:   (){ gtk.container_set_border_width(win, a[0][1]); args(a[1..]) },
            add:            (){ gtk.container_add(win, a[0][1]); args(a[1..]) },
            show:           (){ a[0][1] ? gtk.widget_show(win) : gtk.widget_hide(win); args(a[1..]) },
            show_all:       (){ a[0][1] ? gtk.widget_show_all(win); args(a[1..]) },
            signal_connect: (){ gtk.signal_connect(app, a[0][1], a[1]); args(a[2..]) }
	}[a[0][0]]()
    };
    args(...)
};

gtk_button = (...) {
    widg = gtk.button_new();
    args = (a) {
	a == [] ? widg : {
            always_show_image: (){ gtk.button_set_always_show_image(widg, a[0][1], a[1]); args(a[2..]) },
            label:          (){ gtk.button_set_label(widg, a[0][1]); args(a[1..]) },
	    signal_connect: (){ gtk.signal_connect(widg, a[0][1], a[1]); args(a[2..]) }
	}[a[0][0]]()
    };
    args(...)
};

gtk_grid = (...) {
    grid = gtk.grid_new();
    args = (a) {
	a == [] ? grid : {
            attach:         (){ gtk.grid_attach(grid, a[0][1], a[1]); args(a[2..]) } // note: attach: [,,,], what,
	}[a[0][0]]()
    };
    args(...)
};

gtk_label = (text, ...) {
    widg = gtk.label_new(text);
    args = (a) {
	a == [] ? widg : {
            markup:         (){ gtk.label_set_markup(widg, a[0][1]); args(a[1..]) },
            pattern:        (){ gtk.label_set_pattern(widg, a[0][1]); args(a[1..]) },
            xalign:         (){ gtk.label_set_xalign(widg, a[0][1]); args(a[1..]) },
            yalign:         (){ gtk.label_set_yalign(widg, a[0][1]); args(a[1..]) },
            width_chars:    (){ gtk.label_set_width_chars(widg, a[0][1]); args(a[1..]) },
            max_width_chars:(){ gtk.label_set_max_width_chars(widg, a[0][1]); args(a[1..]) },
            line_wrap:      (){ gtk.label_set_line_wrap(widg, a[0][1]); args(a[1..]) },
            lines:          (){ gtk.label_set_lines(widg, a[0][1]); args(a[1..]) },
            select_region:  (){ gtk.label_select_region(widg, a[0][1], a[1]); args(a[2..]) },
            selectable:     (){ gtk.label_selectable(widg, a[0][1]); args(a[1..]) },
            label:          (){ gtk.label_set_label(widg, a[0][1]); args(a[1..]) },
            angle:          (){ gtk.label_set_angle(widg, a[0][1]); args(a[1..]) }
	}[a[0][0]]()
    };
    args(...)
};

