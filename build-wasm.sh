#!/bin/bash
SOURCES=`find src -name *.cpp -maxdepth 1`
emcc -s WASM=1 -DWASM -s LLD_REPORT_UNDEFINED -s EXTRA_EXPORTED_RUNTIME_METHODS='["cwrap"]' -Isrc $SOURCES -o qak.js