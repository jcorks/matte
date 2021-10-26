all:
	gcc -DMATTE_DEBUG -DMATTE_DEBUG__VM -DMATTE_DEBUG__HEAP -g -fsanitize=address -fsanitize=undefined  -fmax-errors=5 ./*.c ../src/*.c ./system/system.c -o ./matte -lm
