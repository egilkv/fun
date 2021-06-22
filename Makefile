# TAB+S1,9
#
# -finline-functions
# -Og is best for debugging

CCOPTS = -g -Wall -Werror -fmax-errors=10
## CCOPTS = -Og -g -Wall -Werror -fmax-errors=10

# HAVES = -DHAVE_MATH -DHAVE_READLINE
HAVES = -DHAVE_MATH -DHAVE_READLINE -DHAVE_THREADS

CFILES = fun.c compile.c run.c lex.c parse.c cell.c node.c oblist.c cmod.c cfun.c qfun.c err.c m_io.c m_string.c m_math.c m_time.c m_bit.c assoc.c debug.c number.c readline.c
HFILES =	      compile.h run.h lex.h parse.h cell.h node.h oblist.h cmod.h cfun.h qfun.h err.h m_io.h m_string.h m_math.h m_time.h m_bit.h assoc.h debug.h number.h readline.h type.h opt.h

all: fun fun-gtk

fun-gtk: $(CFILES) $(HFILES) m_gtk3.c m_gtk.h set.c set.h Makefile
	gcc $(CCOPTS) -DHAVE_GTK $(HAVES) `pkg-config --cflags gtk+-3.0` $(CFILES) m_gtk3.c set.c \
	    -o fun-gtk `pkg-config --libs gtk+-3.0` -lreadline -lm

fun: $(CFILES) $(HFILES) Makefile
	gcc $(CCOPTS) $(HAVES) $(CFILES) -o fun -lreadline -lm

scan:
	scan-build gcc -g -Wall -Werror -DHAVE_MATH $(CFILES) -o fun -lm

test: fun test.fun
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--log-file=out.txt \
		./fun -PS <test.fun >out 2>&1
	diff -u test.out out

grind: fun
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		./fun -R

#		--verbose \

clean:
	rm -f *~ fun fun-gtk a.out out
