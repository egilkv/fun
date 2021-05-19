# TAB+S1,9
#
#

all: fun fun-gtk

fun-gtk: fun.c lex.c lex.h parse.c parse.h cell.c cell.h oblist.c oblist.h cmod.c cfun.c cmod.h cfun.h eval.c err.c err.h io.c io.h assoc.c assoc.h type.h opt.h gtk3.c
	  gcc -g -Wall -DHAVE_GTK -DHAVE_READLINE `pkg-config --cflags gtk+-3.0` \
	    fun.c lex.c parse.c cell.c oblist.c cmod.c cfun.c eval.c err.c io.c assoc.c gtk3.c \
	    -o fun-gtk `pkg-config --libs gtk+-3.0` -lreadline

fun: fun.c lex.c lex.h parse.c parse.h cell.c cell.h oblist.c oblist.h cmod.c cfun.c cmod.h cfun.h eval.c err.c err.h io.c io.h assoc.c assoc.h type.h opt.h
	  gcc -g -Wall -DHAVE_READLINE fun.c lex.c parse.c cell.c oblist.c cmod.c cfun.c eval.c err.c io.c assoc.c -o fun -lreadline

scan:
	  scan-build gcc -g -Wall fun.c lex.c parse.c cell.c oblist.c cmod.c cfun.c eval.c err.c io.c assoc.c -o fun

test: fun test.fun
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--log-file=out.txt \
		./fun -P <test.fun >out 2>&1
	diff -u test.out out

grind: fun
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		./fun -R

#		--verbose \

clean:
	rm -f *~ fun fun-gtk a.out out
