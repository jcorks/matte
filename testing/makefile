all:
	gcc -DMATTE_DEBUG -DMATTE_DEBUG__HEAP -DMATTE_VERIFY__HEAP_RECYCLE -fsanitize=address -fsanitize=undefined -g -fmax-errors=5 ./*.c ../src/*.c -o ./test_driver -lm
