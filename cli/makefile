


all:
	$(CC) -DMATTE_ROMLESS ./package/make_package.c ./shared.c ../src/*.c ../src/rom/native.c -o ./package/make_package $(LIBS)
	cd ./package && ./make_package
	$(CC) $(DEFINES) $(FLAGS) ./*.c ../src/*.c ../src/rom/native.c -o ./matte $(LIBS)

