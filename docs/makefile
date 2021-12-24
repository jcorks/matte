all:
	emcc ./synbox.c ../src/*.c -o ../matte_synbox_c.js -s EXPORTED_FUNCTIONS='["_matte_js_syntax_analysis"]' -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'
