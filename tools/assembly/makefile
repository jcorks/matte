all:
	gcc matteasm2bytecode.c -g -fsanitize=address -fsanitize=undefined -o m2b
	gcc bytecode2matteasm.c ../../src/matte_string.c -g -o b2m
