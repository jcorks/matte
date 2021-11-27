all:
	gcc -DMATTE_USE_SYSTEM_EXTENSIONS -DMATTE_DEBUG -pthread -g -fsanitize=address -fsanitize=undefined  -fmax-errors=5 ./*.c ../src/*.c ../src/rom/system/system.c -o ./matte -lm
