# TAB+S1,9
#
#

fun: fun.c lex.c lex.h parse.c parse.h cell.c cell.h oblist.c oblist.h cfun.c cfun.h
	gcc -g fun.c lex.c parse.c cell.c oblist.c cfun.c -o fun

test: fun test.fun
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--log-file=out.txt \
		./fun < test.fun

#		--verbose \

grind: fun
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		./fun

clean:
	rm *~ fun a.out
