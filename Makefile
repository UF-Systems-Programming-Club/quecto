.PHONY: tests

main: src/*.c include/*.h
	gcc -I include src/*.c -o main

tests: tests/test.c include/*.h src/tokenizer.c
	gcc -I include tests/test.c src/tokenizer.c -o test
