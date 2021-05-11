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

#define pushchar(c) ungetc(c,stdin)

static item *pushback = 0;
static item *gotchar(int c, item *it);
static item *gotsymbol(char c, item *it);

item *lexical() {
    item *it;
    if (pushback) {
	it = pushback;
        pushback = 0;
    } else {
        it = gotchar(getchar(), (item *)0);
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

static item *goteof(char c, item *it) {
    // assume eof is returned more than once
    return it;
}

static item *gotdigit(char c, item *it) {
    if (it) {
        if (it->type == it_SYMBOL) {
            return gotsymbol(c, it);
        }
        if (it->type != it_INTEGER) {
            pushchar(c);
            return it;
        }
    } else {
        it = newitem(it_INTEGER);
    }
    it->ivalue = it->ivalue*10 + c-'0';
    return gotchar(getchar(), it);
}

static item *gotsymbol(char c, item *it) {
    int n;
    if (it) {
        if (it->type != it_SYMBOL) {
            pushchar(c);
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
    return gotchar(getchar(), it);
}

static item *gotstring(char c, item *it) {
    int n = 0;
    if (it) {
        if (it->type != it_STRING) {
            pushchar(c);
            return it;
        }
        if (c == '"') { // end of string?
            return it;
        } 
        if (c == '\\') {
            int c2 = getchar();
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
                return gotstring(getchar(), it);
            }
        } else if (iscntrl(c)) {
            // TODO error
            if (c == '\r' || c == '\n') {
                error_lex("string missing trailing quote", -1);
                pushchar(c);
                return it;
            }
            error_lex("bad control character in string, ignored", c);
            return gotstring(getchar(), it);
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
    return gotstring(getchar(), it);
}

static item *gotspace(char c, item *it) {
    return it ? it : gotchar(getchar(), NULL);
}

static item *gotdefault(char c, item *it) {
    error_lex("bad character, ignored", c);
    return it ? it : gotchar(getchar(), 0);
}

static item *goteq(char c, item *it) {
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
            pushchar(c);
            break;
        }
    } else {
        it = gotchar(getchar(), newitem(it_EQ));
    }
    return it;
}

static item *gotamp(char c, item *it) {
    if (it) {
        if (it->type == it_AMP) {
            it->type = it_AND;
        } else {
            pushchar(c);
        }
    } else {
        it = gotchar(getchar(), newitem(it_AMP));
    }
    return it;
}

static item *gotstop(char c, item *it) {
    if (it) {
        if (it->type == it_STOP) {
            it->type = it_ELIP;
        } else {
            pushchar(c);
        }
    } else {
        it = gotchar(getchar(), newitem(it_STOP));
    }
    return it;
}

static item *gotdiv(char c, item *it) {
    if (it) {
        if (it->type == it_DIV) { // comment until end of line
            dropitem(it);
            // TODO iteration
            int c;
            do {
                c = getchar();
                if (c < 0) return 0; // end of file
            } while (c != '\r' && c != '\n');
            // end of line
            it = gotchar(c, 0);
        } else {
            pushchar(c);
        }
    } else {
        it = gotchar(getchar(), newitem(it_DIV));
    }
    return it;
}

static item *gotplain(char c, token type, item *it) {
    if (it) pushchar(c);
    else it = newitem(type);
    return it;
}

static item *gotlt(char c, item *it)    { return gotplain(c, it_LT, it); }
static item *gotgt(char c, item *it)    { return gotplain(c, it_GT, it); }
static item *gotnot(char c, item *it)   { return gotplain(c, it_NOT, it); }
static item *gotquote(char c, item *it) { return gotplain(c, it_QUOTE, it); }
static item *gotplus(char c, item *it)  { return gotplain(c, it_PLUS, it); }
static item *gotminus(char c, item *it) { return gotplain(c, it_MINUS, it); }
static item *gotmult(char c, item *it)  { return gotplain(c, it_MULT, it); }
static item *gotcomma(char c, item *it) { return gotplain(c, it_COMMA, it); }
static item *gotcolon(char c, item *it) { return gotplain(c, it_COLON, it); }
static item *gotsemi(char c, item *it)  { return gotplain(c, it_SEMI, it); }
static item *gotquest(char c, item *it) { return gotplain(c, it_QUEST, it); }
static item *gotlpar(char c, item *it)  { return gotplain(c, it_LPAR, it); }
static item *gotrpar(char c, item *it)  { return gotplain(c, it_RPAR, it); }
static item *gotlbrk(char c, item *it)  { return gotplain(c, it_LBRK, it); }
static item *gotrbrk(char c, item *it)  { return gotplain(c, it_RBRK, it); }
static item *gotlbrc(char c, item *it)  { return gotplain(c, it_LBRC, it); }
static item *gotrbrc(char c, item *it)  { return gotplain(c, it_RBRC, it); }

// return item, or NULL is end of file
static item *gotchar(int c, item *it) {
    if (c < 0) {
	return goteof(c, it);
    } else if (isdigit(c)) {
	return gotdigit(c, it);
    } else if (isalnum(c)) {
        return gotsymbol(c, it);
    } else switch (c) {
    case '#':
        return gotsymbol(c, it);
    case '\'':
        return gotquote(c, it);
    case '"':
        return gotstring(c, it);
    case '+':
        return gotplus(c, it);
    case '*':
        return gotmult(c, it);
    case '-':
        return gotminus(c, it);
    case '/':
        return gotdiv(c, it);
    case '<':
        return gotlt(c, it);
    case '>':
        return gotgt(c, it);
    case '!':
        return gotnot(c, it);
    case '&':
        return gotamp(c, it);
    case '=':
        return goteq(c, it);
    case '.':
        return gotstop(c, it);
    case ',':
        return gotcomma(c, it);
    case ':':
        return gotcolon(c, it);
    case ';':
	return gotsemi(c, it);
    case '?':
        return gotquest(c, it);
    case '(':
        return gotlpar(c, it);
    case ')':
        return gotrpar(c, it);
    case '[':
        return gotlbrk(c, it);
    case '\\':
        return gotstring(c, it);
    case ']':
        return gotrbrk(c, it);
    case '{':
        return gotlbrc(c, it);
    case '}':
        return gotrbrc(c, it);

    // TODO '|'

    case '\r':
    case '\n':
    case '\t':
    case ' ':
	return gotspace(c, it);
    }
    return gotdefault(c, it);
}
