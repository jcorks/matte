

all:
	rm -f ./*.gcda
	gcc -g -O0 -fsanitize=address -fsanitize=undefined  -DMATTE_DEBUG -DMATTE_DEBUG__HEAP -pthread -fmax-errors=5 -fprofile-arcs -ftest-coverage ./*.c ../src/*.c ../src/rom/native.c -o ./test_driver -lm



coverage-report:
	gcovr -r ../src/ --object-directory . --html --html-details -o index.html
