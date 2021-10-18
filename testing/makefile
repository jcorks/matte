all:
	gcc -fsanitize=address -fsanitize=undefined -DMATTE_DEBUG__HEAP -DMATTE_DEBUG -g -fmax-errors=5 ./*.c ../src/*.c -o ./test_driver -lm
