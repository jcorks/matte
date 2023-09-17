with-extensions:
	$(CC) -std=c99 -g -DMATTE_USE_SYSTEM_EXTENSIONS makerom.c ../matte_string.c ../matte_array.c ../matte_table.c ../matte_compiler.c ../matte_compiler__syntax_graph.c -o makerom
	
no-extensions:
	$(CC) -std=c99 -g makerom.c -DMATTE_DEBUG ../matte_string.c ../matte_array.c ../matte_table.c ../matte_compiler.c ../matte_compiler__syntax_graph.c -o makerom
