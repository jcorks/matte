all:
	gcc -DMATTE_DEBUG  -fsanitize=address -fsanitize=undefined -DMATTE_DEBUG__HEAP -g -fmax-errors=5 ./*.c ../src/*.c ./system/system.c -o ./matte -lm
