/*  TAB-P
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "type.h"
#include "opt.h"
#include "readline.h"
#ifdef HAVE_READLINE
#include "oblist.h"
#endif

#ifdef HAVE_READLINE
// https://thoughtbot.com/blog/tab-completion-in-gnu-readline
// if there are no possible completions, return NULL.
// if there is one possible completion, return an array 
// containing that completion, followed by a NULL value.
// if there are two or more possibilities, return an array
// containing the longest common prefix of all the options, 
// followed by each of the possible completions, followed
// by a NULL value.
static char **readline_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1; // do not fall back to filename completion
//  return rl_completion_matches(text, character_name_generator);
    return rl_completion_matches(text, oblist_search);
}
#endif // HAVE_READLINE

void read_line_init() {
#ifdef HAVE_READLINE
    // disable tab completion
    // rl_bind_key('\t', rl_insert);

    // have our own TAB-completion
    rl_attempted_completion_function = readline_completion;

    // let readline know our quote characters etc
    // rl_completer_quote_characters = "\"'";
    // rl_completer_word_break_characters = " ";
#endif
}

int read_line(char **linebufp, const char *show_prompt) {
    ssize_t linelen = 0;
#ifdef HAVE_READLINE
    if (!opt_noreadline) {
        *linebufp = readline(show_prompt ? show_prompt : "    ");
        if (!(*linebufp)) {
            return -1; // end of file
        }
        linelen = strlen(*linebufp);
        if (linelen > 0) {
            add_history(*linebufp);
        }
    } else
#endif
    {
        // prompt
        fprintf(stdout, "%s", show_prompt ? show_prompt : "    ");
        fflush(stdout);
        *linebufp = line_from_file(stdin, &linelen);
        if (!*linebufp) {
            return -1; // end of file
        }
    }
    return linelen;
}

// return allocated line from stdin or file
// *lenp does not include trailing newline
char *line_from_file(FILE *f, ssize_t *lenp) {
    size_t buflen = 1;
    ssize_t len;
    char *buf = malloc(1); // if NULL, getline will malloc a huge buffer
    if ((len = getline(&buf, &buflen, f)) < 0) {
        // usually an eof but could be other type of error
        // TODO errno is error
        free(buf);
        *lenp = 0;
        return NULL;
    }
    assert(buf);
    if (len > 0 && buf[len-1] == '\n') --len; // skip trailing newline
    *lenp = len;
    return buf;
}
