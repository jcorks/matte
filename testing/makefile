all:
	rm -f ./*.gcda
	gcc -DMATTE_DEBUG -fmax-errors=5 -fprofile-arcs -ftest-coverage ./*.c ../src/*.c -o ./test_driver -lm

coverage-report:
	gcovr -r ../src/ --object-directory . --html --html-details -o index.html
