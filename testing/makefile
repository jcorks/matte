

all:
	rm -f ./*.gcda
	gcc -g -DMATTE_DEBUG -DMATTE_DEBUG__HEAP  -fsanitize=address -fsanitize=undefined -DMATTE_USE_SYSTEM_EXTENSIONS -pthread -fmax-errors=5 -fprofile-arcs -ftest-coverage ./*.c ../src/*.c ../src/rom/native.c -o ./test_driver -lm



coverage-report:
	gcovr -r ../src/ --object-directory . --html --html-details -o index.html
