all:
	gcc -DMATTE_USE_SYSTEM_EXTENSIONS -fsanitize=address -fsanitize=undefined -g -fmax-errors=5 ./*.c ../src/*.c ../src/rom/system/system.c -o ./matte -lm
