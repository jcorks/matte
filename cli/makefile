all:
	gcc -fsanitize=address -fsanitize=undefined -DMATTE_DEBUG -g -fmax-errors=5 ./*.c ../src/*.c ./system/system.c -o ./matte -lm
