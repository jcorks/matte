all:
	gcc -DMATTE_DEBUG -DMATTE_DEBUG__COMPILER  -fsanitize=address -fsanitize=undefined -g -fmax-errors=5 ./*.c ../src/*.c -o ./matte -lm
