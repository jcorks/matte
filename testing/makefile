

all:
	rm -f ./*.gcda
	gcc -fsanitize=address -fsanitize=undefined -O0 -DMATTE_DEBUG -DMATTE_DEBUG__STORE -DMATTE_DEBUG__STORE_LEVEL_2  -pthread -fmax-errors=5 -fprofile-arcs -ftest-coverage ./*.c ../src/*.c ../src/rom/native.c -o ./test_driver -lm



coverage-report:
	gcovr -r ../src/ --object-directory . --html --html-details -o index.html
