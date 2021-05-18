/* TAB-P
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#include "oblist.h"
#endif

#include "lex.h"
#include "err.h"
#include "opt.h"

static item *pushback = 0;
static item *gotchar(int c, item *it, lxfile *in);
static item *gotsymbol(char c, item *it, lxfile *in);

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

void lxfile_init(lxfile *in, FILE *f) {
    in->f = f;
    if (f == stdin) {
        in->is_terminal = isatty(fileno(stdin));
    } else {
        in->is_terminal = 0;
    }
    in->is_eof = 0;
    in->lineno = 1;
    in->index = 0;
    in->linebuf = NULL;
    in->linelen = 0;

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

// BUG: static string
const char *lxfile_info(lxfile *in) {
    static char infobuf[81];
    snprintf(infobuf, sizeof(infobuf)-1, " at %d:%ld", in->lineno, in->index);
    return infobuf;
}

// return allocated line from stdin or file
// *lenp does not include trailing newline
char *lex_getline(FILE *f, ssize_t *lenp) {
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

// TODO inline
static void lxungetc(char c, lxfile *in) {
    if (in->is_terminal) {
        assert(in->index > 0);
        --(in->index);
    } else {
        --(in->index); // TODO handle -1 for end of previous line
        ungetc(c == '\n' ? ' ' : c, in->f);
    }
}

// TODO inline
static int lxgetc(lxfile *in) {
    int c;
    if (in->is_terminal) {
        while (in->index >= in->linelen) { // reached end of line?
            if (in->index == in->linelen) {
                if (in->linebuf) {
                    ++(in->index);
                    return '\n';
                } else {
                    if (in->is_eof) return -1;
                }
            }
            // reached end of line, ask for next
            if (in->linebuf) {
                free(in->linebuf);
            }
            else if (in->lineno == 1) in->lineno = 0; // fix line number
            in->index = 0;

#ifdef HAVE_READLINE
            if (!opt_noreadline) {
                in->linelen = 0;
                in->linebuf = readline("\n--> ");
                if (!(in->linebuf)) {
                    in->is_eof = 1;
                    return -1; // end of file
                }
                in->linelen = strlen(in->linebuf);
                if (in->linelen > 0) {
                    add_history(in->linebuf);
                }
            } else
#endif
            {
                // prompt
                fprintf(stdout, "\n--> ");
                fflush(stdout);
                in->linebuf = lex_getline(in->f, &(in->linelen));
                if (!(in->linebuf)) {
                    in->is_eof = 1;
                    return -1; // end of file
                }
            }
            ++(in->lineno);
        }
        if (!in->linebuf) {
            return -1;
        }
        c = in->linebuf[(in->index)++];
    } else {
        c = fgetc(in->f);
        switch (c) {
        case '\n':
            ++(in->lineno);
            in->index = 0;
            break;
        case '\r':
            break;
        case -1:
            in->is_eof = 1;
            break;
        default:
            ++(in->index);
            break;
        }
    }
    return c;
}

static item *nextchar(item *it, lxfile *in) {
    return gotchar(lxgetc(in), it, in);
}

item *lexical(lxfile *in) {
    item *it;
    if (pushback) {
	it = pushback;
        pushback = 0;
    } else {
        it = nextchar(NULL, in);
    }
    return it;
}

void dropitem(item *it) {
    if (it) {
        if (it->svalue) free(it->svalue);
        free(it);
    }
}

void pushitem(item *it) {
    // TODO do we really need it???
    if (!pushback) {
        pushback = it;
    } else {
        assert(0);
    }
}

static item *newitem(token type) {
    item *it = malloc(sizeof(item));
    assert(it);
    memset(it, 0, sizeof(item));
    it->type = type;
    return it;
}

static item *goteof(char c, item *it, lxfile *in) {
    // assume eof is returned more than once
    return it;
}

static item *gotdigit(char c, item *it, lxfile *in) {
    if (it) {
        if (it->type == it_SYMBOL) {
            return gotsymbol(c, it, in);
        }
        if (it->type != it_INTEGER) {
            lxungetc(c, in);
            return it;
        }
    } else {
        it = newitem(it_INTEGER);
    }
    it->ivalue = it->ivalue*10 + c-'0';
    return nextchar(it, in);
}

static item *gotsymbol(char c, item *it, lxfile *in) {
    int n;
    if (it) {
        if (it->type != it_SYMBOL) {
            lxungetc(c, in);
            return it;
        }
        assert(it->svalue);
        n = it->slen;
    } else {
        it = newitem(it_SYMBOL);
        n = 0;
    }
    it->svalue = realloc(it->svalue, n+2);
    assert(it->svalue);
    it->svalue[n] = c;
    it->svalue[n+1] = '\0';
    it->slen = n+1;
    return nextchar(it, in);
}

static item *gotstring(char c, item *it, lxfile *in) {
    int n = 0;
    if (it) {
        if (it->type != it_STRING) {
            lxungetc(c, in);
            return it;
        }
        if (c == '"') { // end of string?
            return it;
        } 
        if (c == '\\') {
            int c2 = lxgetc(in);
            if (c2 < 0) {
		error_lex(lxfile_info(in), "unterminated string", -1);
                return it;
            }
            switch (c2) {
            case '"':  // TODO surely more
            case '\'':
            case '\\':
            case '?':
                c = c2; // next as is
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case '0': // TODO implement https://en.wikipedia.org/wiki/Escape_sequences_in_C
            case 'x':
            case 'u':
            case 'U':
            default: // TODO implement
		error_lex(lxfile_info(in), "unknown \\-escape, ignored", c2);
                return gotstring(lxgetc(in), it, in);
            }
        } else if (iscntrl(c)) {
            // TODO error
            if (c == '\n' || c == '\r') {
		error_lex(lxfile_info(in), "string missing trailing quote", -1);
                lxungetc(c, in);
                return it;
            }
	    error_lex(lxfile_info(in), "bad control character in string, ignored", c);
            return gotstring(lxgetc(in), it, in);
        }
        // string continues
        assert(it->svalue);
        n = 1 + it->slen;
        it->svalue = realloc(it->svalue, n+1);
        assert(it->svalue);
        it->svalue[n-1] = c;
    } else {
        // start of string
        assert(c == '"');
        it = newitem(it_STRING);
        it->svalue = malloc(1);
        assert(it->svalue);
    }
    it->svalue[n] = '\0';
    it->slen = n;
    return gotstring(lxgetc(in), it, in);
}

static item *gotspace(char c, item *it, lxfile *in) {
    return it ? it : nextchar(NULL, in);
}

static item *gotdefault(char c, item *it, lxfile *in) {
    error_lex(lxfile_info(in), "bad character, ignored", c);
    return it ? it : nextchar(NULL, in);
}

static item *goteq(char c, item *it, lxfile *in) {
    if (it) {
        switch (it->type) {
        case it_LT:
            it->type = it_LTEQ;
            break;
        case it_GT:
            it->type = it_GTEQ;
            break;
        case it_NOT:
            it->type = it_NTEQ;
            break;
        case it_EQ:
            it->type = it_EQEQ;
            break;
        default:
            lxungetc(c, in);
            break;
        }
    } else {
        it = nextchar(newitem(it_EQ), in);
    }
    return it;
}

static item *gotamp(char c, item *it, lxfile *in) {
    if (it) {
        if (it->type == it_AMP) {
            it->type = it_AND;
        } else {
            lxungetc(c, in);
        }
    } else {
        it = nextchar(newitem(it_AMP), in);
    }
    return it;
}

static item *gotbar(char c, item *it, lxfile *in) {
    if (it) {
        if (it->type == it_BAR) {
            it->type = it_OR;
        } else {
            lxungetc(c, in);
        }
    } else {
        it = nextchar(newitem(it_BAR), in);
    }
    return it;
}

static item *gotstop(char c, item *it, lxfile *in) {
    if (it) {
        if (it->type == it_STOP) {
            it->type = it_ELIP;
        } else {
            lxungetc(c, in);
        }
    } else {
        it = nextchar(newitem(it_STOP), in);
    }
    return it;
}

static item *gotnot(char c, item *it, lxfile *in) {
    if (it) lxungetc(c, in);
    else it = nextchar(newitem(it_NOT), in);
    return it;
}

static item *gotdiv(char c, item *it, lxfile *in) {
    if (it) {
        if (it->type == it_DIV) { // double slash, comment until end of line
            dropitem(it);
            // TODO iteration
            int c;
            do {
                c = lxgetc(in);
                if (c < 0) return 0; // end of file
            } while (c != '\r' && c != '\n');
            // end of line
            it = gotchar(c, NULL, in);
        } else {
            lxungetc(c, in);
        }
    } else {
        it = nextchar(newitem(it_DIV), in); // reqd for peek into next
    }
    return it;
}

static item *gotmult(char c, item *it, lxfile *in) {
    if (it) {
        if (it->type == it_DIV) { // slash-star, comment until end of line
            dropitem(it);
            // TODO iteration
            int c = 0;
            int c0;
            do {
                c0 = c;
                c = lxgetc(in);
                if (c < 0) return 0; // end of file
            } while (c0 != '*' || c != '/');
            it = nextchar(NULL, in);
        } else {
            lxungetc(c, in);
        }
    } else {
        it = newitem(it_MULT);
    }
    return it;
}

static item *gotplain(char c, token type, item *it, lxfile *in) {
    if (it) lxungetc(c, in);
    else it = newitem(type);
    return it;
}

static item *gotlt(char c, item *it, lxfile *in)    { return gotplain(c, it_LT, it, in); }
static item *gotgt(char c, item *it, lxfile *in)    { return gotplain(c, it_GT, it, in); }
static item *gotquote(char c, item *it, lxfile *in) { return gotplain(c, it_QUOTE, it, in); }
static item *gotplus(char c, item *it, lxfile *in)  { return gotplain(c, it_PLUS, it, in); }
static item *gotminus(char c, item *it, lxfile *in) { return gotplain(c, it_MINUS, it, in); }
static item *gotcomma(char c, item *it, lxfile *in) { return gotplain(c, it_COMMA, it, in); }
static item *gotcolon(char c, item *it, lxfile *in) { return gotplain(c, it_COLON, it, in); }
static item *gotsemi(char c, item *it, lxfile *in)  { return gotplain(c, it_SEMI, it, in); }
static item *gotquest(char c, item *it, lxfile *in) { return gotplain(c, it_QUEST, it, in); }
static item *gotlpar(char c, item *it, lxfile *in)  { return gotplain(c, it_LPAR, it, in); }
static item *gotrpar(char c, item *it, lxfile *in)  { return gotplain(c, it_RPAR, it, in); }
static item *gotlbrk(char c, item *it, lxfile *in)  { return gotplain(c, it_LBRK, it, in); }
static item *gotrbrk(char c, item *it, lxfile *in)  { return gotplain(c, it_RBRK, it, in); }
static item *gotlbrc(char c, item *it, lxfile *in)  { return gotplain(c, it_LBRC, it, in); }
static item *gotrbrc(char c, item *it, lxfile *in)  { return gotplain(c, it_RBRC, it, in); }

// return item, or NULL is end of file
static item *gotchar(int c, item *it, lxfile *in) {
    if (c < 0) {
        return goteof(c, it, in);
    } else if (isdigit(c)) {
        return gotdigit(c, it, in);
    } else if (isalnum(c)) {
        return gotsymbol(c, it, in);
    } else switch (c) {
    case '!':
        return gotnot(c, it, in);
    case '"':
        return gotstring(c, it, in);
    case '#':
        return gotsymbol(c, it, in);
    case '$':
        break; // not used
    case '%':
        break; // not used
    case '&':
        return gotamp(c, it, in);
    case '\'':
        return gotquote(c, it, in);
    case '(':
        return gotlpar(c, it, in);
    case ')':
        return gotrpar(c, it, in);
    case '*':
        return gotmult(c, it, in);
    case '+':
        return gotplus(c, it, in);
    case ',':
        return gotcomma(c, it, in);
    case '-':
        return gotminus(c, it, in);
    case '.':
        return gotstop(c, it, in);
    case '/':
        return gotdiv(c, it, in);
    case ':':
        return gotcolon(c, it, in);
    case ';':
        return gotsemi(c, it, in);
    case '<':
        return gotlt(c, it, in);
    case '=':
        return goteq(c, it, in);
    case '>':
        return gotgt(c, it, in);
    case '?':
        return gotquest(c, it, in);
    case '@':
        break; // not used
    case '[':
        return gotlbrk(c, it, in);
    case '\\':
        return gotstring(c, it, in);
    case ']':
        return gotrbrk(c, it, in);
    case '^':
        break; // not used
    case '_':
        return gotsymbol(c, it, in);
    case '`':
        break; // not used
    case '{':
        return gotlbrc(c, it, in);
    case '|':
        return gotbar(c, it, in);
    case '}':
        return gotrbrc(c, it, in);
    case '~':
        break; // not used

    case '\r':
    case '\n':
    case '\t':
    case ' ':
        return gotspace(c, it, in);
    }
    return gotdefault(c, it, in);
}
