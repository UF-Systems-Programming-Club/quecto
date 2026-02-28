.PHONY: tests

main: src/*.c include/*.h
	gcc -I include src/*.c -o main

debug: src/*.c
	gcc -fsanitize=address -g -I include src/*.c -o main

tests: tests/test.c include/*.h src/tokenizer.c
	gcc -I include tests/test.c src/tokenizer.c -o test

out: out.S
	nasm -felf64 -o out.o out.S
	ld -o out out.o
	rm out.o
