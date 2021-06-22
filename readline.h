/*  TAB-P
 *
 */

void read_line_init();

int read_line(char **linebufp, const char *show_prompt);

char *line_from_file(FILE *f, ssize_t *lenp);
