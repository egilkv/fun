/* TAB-P
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "lex.h"
#include "err.h"

static item *pushback = 0;
static item *gotchar(int c, item *it, FILE *in);
static item *gotsymbol(char c, item *it, FILE *in);

item *lexical(FILE *in) {
    item *it;
    if (pushback) {
	it = pushback;
        pushback = 0;
    } else {
        it = gotchar(fgetc(in), NULL, in);
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

static item *goteof(char c, item *it, FILE *in) {
    // assume eof is returned more than once
    return it;
}

static item *gotdigit(char c, item *it, FILE *in) {
    if (it) {
        if (it->type == it_SYMBOL) {
            return gotsymbol(c, it, in);
        }
        if (it->type != it_INTEGER) {
            ungetc(c,in);
            return it;
        }
    } else {
        it = newitem(it_INTEGER);
    }
    it->ivalue = it->ivalue*10 + c-'0';
    return gotchar(fgetc(in), it, in);
}

static item *gotsymbol(char c, item *it, FILE *in) {
    int n;
    if (it) {
        if (it->type != it_SYMBOL) {
            ungetc(c,in);
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
    return gotchar(fgetc(in), it, in);
}

static item *gotstring(char c, item *it, FILE *in) {
    int n = 0;
    if (it) {
        if (it->type != it_STRING) {
            ungetc(c,in);
            return it;
        }
        if (c == '"') { // end of string?
            return it;
        } 
        if (c == '\\') {
            int c2 = fgetc(in);
            if (c2 < 0) {
                error_lex("unterminated string", -1);
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
                error_lex("unknown \\-escape, ignored", c2);
                return gotstring(fgetc(in), it, in);
            }
        } else if (iscntrl(c)) {
            // TODO error
            if (c == '\r' || c == '\n') {
                error_lex("string missing trailing quote", -1);
                ungetc(c,in);
                return it;
            }
            error_lex("bad control character in string, ignored", c);
            return gotstring(fgetc(in), it, in);
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
    return gotstring(fgetc(in), it, in);
}

static item *gotspace(char c, item *it, FILE *in) {
    return it ? it : gotchar(fgetc(in), NULL, in);
}

static item *gotdefault(char c, item *it, FILE *in) {
    error_lex("bad character, ignored", c);
    return it ? it : gotchar(fgetc(in), NULL, in);
}

static item *goteq(char c, item *it, FILE *in) {
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
            ungetc(c,in);
            break;
        }
    } else {
        it = gotchar(fgetc(in), newitem(it_EQ), in);
    }
    return it;
}

static item *gotamp(char c, item *it, FILE *in) {
    if (it) {
        if (it->type == it_AMP) {
            it->type = it_AND;
        } else {
            ungetc(c,in);
        }
    } else {
        it = gotchar(fgetc(in), newitem(it_AMP), in);
    }
    return it;
}

static item *gotstop(char c, item *it, FILE *in) {
    if (it) {
        if (it->type == it_STOP) {
            it->type = it_ELIP;
        } else {
            ungetc(c,in);
        }
    } else {
        it = gotchar(fgetc(in), newitem(it_STOP), in);
    }
    return it;
}

static item *gotdiv(char c, item *it, FILE *in) {
    if (it) {
        if (it->type == it_DIV) { // comment until end of line
            dropitem(it);
            // TODO iteration
            int c;
            do {
                c = fgetc(in);
                if (c < 0) return 0; // end of file
            } while (c != '\r' && c != '\n');
            // end of line
            it = gotchar(c, NULL, in);
        } else {
            ungetc(c,in);
        }
    } else {
        it = gotchar(fgetc(in), newitem(it_DIV), in);
    }
    return it;
}

static item *gotplain(char c, token type, item *it, FILE *in) {
    if (it) ungetc(c,in);
    else it = newitem(type);
    return it;
}

static item *gotlt(char c, item *it, FILE *in)    { return gotplain(c, it_LT, it, in); }
static item *gotgt(char c, item *it, FILE *in)    { return gotplain(c, it_GT, it, in); }
static item *gotnot(char c, item *it, FILE *in)   { return gotplain(c, it_NOT, it, in); }
static item *gotquote(char c, item *it, FILE *in) { return gotplain(c, it_QUOTE, it, in); }
static item *gotplus(char c, item *it, FILE *in)  { return gotplain(c, it_PLUS, it, in); }
static item *gotminus(char c, item *it, FILE *in) { return gotplain(c, it_MINUS, it, in); }
static item *gotmult(char c, item *it, FILE *in)  { return gotplain(c, it_MULT, it, in); }
static item *gotcomma(char c, item *it, FILE *in) { return gotplain(c, it_COMMA, it, in); }
static item *gotcolon(char c, item *it, FILE *in) { return gotplain(c, it_COLON, it, in); }
static item *gotsemi(char c, item *it, FILE *in)  { return gotplain(c, it_SEMI, it, in); }
static item *gotquest(char c, item *it, FILE *in) { return gotplain(c, it_QUEST, it, in); }
static item *gotlpar(char c, item *it, FILE *in)  { return gotplain(c, it_LPAR, it, in); }
static item *gotrpar(char c, item *it, FILE *in)  { return gotplain(c, it_RPAR, it, in); }
static item *gotlbrk(char c, item *it, FILE *in)  { return gotplain(c, it_LBRK, it, in); }
static item *gotrbrk(char c, item *it, FILE *in)  { return gotplain(c, it_RBRK, it, in); }
static item *gotlbrc(char c, item *it, FILE *in)  { return gotplain(c, it_LBRC, it, in); }
static item *gotrbrc(char c, item *it, FILE *in)  { return gotplain(c, it_RBRC, it, in); }

// return item, or NULL is end of file
static item *gotchar(int c, item *it, FILE *in) {
    if (c < 0) {
        return goteof(c, it, in);
    } else if (isdigit(c)) {
        return gotdigit(c, it, in);
    } else if (isalnum(c)) {
        return gotsymbol(c, it, in);
    } else switch (c) {
    case '#':
        return gotsymbol(c, it, in);
    case '\'':
        return gotquote(c, it, in);
    case '"':
        return gotstring(c, it, in);
    case '+':
        return gotplus(c, it, in);
    case '*':
        return gotmult(c, it, in);
    case '-':
        return gotminus(c, it, in);
    case '/':
        return gotdiv(c, it, in);
    case '<':
        return gotlt(c, it, in);
    case '>':
        return gotgt(c, it, in);
    case '!':
        return gotnot(c, it, in);
    case '&':
        return gotamp(c, it, in);
    case '=':
        return goteq(c, it, in);
    case '.':
        return gotstop(c, it, in);
    case ',':
        return gotcomma(c, it, in);
    case ':':
        return gotcolon(c, it, in);
    case ';':
        return gotsemi(c, it, in);
    case '?':
        return gotquest(c, it, in);
    case '(':
        return gotlpar(c, it, in);
    case ')':
        return gotrpar(c, it, in);
    case '[':
        return gotlbrk(c, it, in);
    case '\\':
        return gotstring(c, it, in);
    case ']':
        return gotrbrk(c, it, in);
    case '{':
        return gotlbrc(c, it, in);
    case '}':
        return gotrbrc(c, it, in);

    // TODO '|'

    case '\r':
    case '\n':
    case '\t':
    case ' ':
        return gotspace(c, it, in);
    }
    return gotdefault(c, it, in);
}
