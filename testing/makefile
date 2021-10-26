all:
	gcc -DMATTE_DEBUG -g -fmax-errors=5 -fprofile-arcs -ftest-coverage ./*.c ../src/*.c -o ./test_driver -lm
coverage-report:
	gcovr --html --html-details
