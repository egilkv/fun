# TAB+S1,9
#
#

fun2: fun2.c lex.c lex.h parse.c parse.h cell.c cell.h oblist.c
	gcc -g fun2.c lex.c parse.c cell.c oblist.c -o fun2

test: fun2 test.f2
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--log-file=out.txt \
		./fun2 < test.f2

#		--verbose \

grind: fun2
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		./fun2

clean:
	rm *~ fun2
