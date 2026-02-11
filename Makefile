main: src/*.c include/*.h
	gcc -I include src/*.c -o main

tests: tests/test.c include/*.h src/lexer.c
	gcc -I include tests/test.c src/lexer.c -o test
