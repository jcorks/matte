with-extensions:
	$(CC) -g -fsanitize=address -fsanitize=undefined makerom.c -DMATTE_USE_SYSTEM_EXTENSIONS ../matte_string.c ../matte_array.c ../matte_table.c ../matte_compiler.c ../matte_compiler__syntax_graph.c -o makerom
	
no-extensions:
	$(CC) -g makerom.c -DMATTE_DEBUG ../matte_string.c ../matte_array.c ../matte_table.c ../matte_compiler.c ../matte_compiler__syntax_graph.c -o makerom
