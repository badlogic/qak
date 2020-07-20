#!/bin/bash
SOURCES=`find src -name *.cpp -maxdepth 1`
emcc -O0 -DWASM \
	-s WASM=1 \
	-s LLD_REPORT_UNDEFINED \
	-s EXTRA_EXPORTED_RUNTIME_METHODS='["cwrap", "allocateUTF8", "UTF8ArrayToString"]' \
	--extern-pre-js wasm/qak_pre.js \
	--extern-post-js wasm/qak_post.js \
	-Isrc $SOURCES \
	-o wasm/qak.js