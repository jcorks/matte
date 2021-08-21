all:
	gcc -DMATTE_DEBUG -fsanitize=address -fsanitize=undefined -DMATTE_VERIFY__HEAP_RECYCLE -g -fmax-errors=5 ./*.c ../src/*.c -o ./test_driver -lm
