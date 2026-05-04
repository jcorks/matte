all:
	gcc ../../src/*.c -DMATTE_NO_EXTENSIONS -DMATTE_ROMLESS ./main.c -g -fsanitize=address -fsanitize=undefined  -o ./resilience -lm

valgrind:
	gcc ../../src/*.c -DMATTE_NO_EXTENSIONS -DMATTE_ROMLESS ./main.c -g  -o ./resilience -lm
