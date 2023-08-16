all:
	emcc -O2 ./synbox.c ../src/*.c ../src/rom/native.c -o matte_synbox_c.js -s EXPORTED_FUNCTIONS='["_matte_js_syntax_analysis", "_matte_js_run", "_matte_js_repl"]' -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'
