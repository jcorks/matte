all:
	gcc -g -fsanitize=address -fsanitize=undefined -DMATTE_USE_SYSTEM_EXTENSIONS -fmax-errors=5 ./*.c ../src/*.c ../src/rom/native.c -o ./matte -lm -lpthread
