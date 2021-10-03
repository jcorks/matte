all:
	gcc -DMATTE_DEBUG -DMATTE_DEBUG__HEAP -DMATTE_DEBUG__VM -DMATTE_VERIFY__HEAP -fsanitize=address -fsanitize=undefined -g -fmax-errors=5 ./*.c ../src/*.c ./system/system.c -o ./matte -lm
