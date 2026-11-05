#!/bin/sh
# Compila la demo nel browser: simulatore + Iride in WebAssembly (Emscripten).
# Uso: sh simulator/web/build.sh [cartella di uscita, di serie site/]
set -e
cd "$(dirname "$0")/.."
OUT="${1:-../site}"
mkdir -p "$OUT"
emcc -std=gnu99 -O2 -w -DIRIDE_WEB -Ishim preview.c -lm -o "$OUT/iride.js" \
    -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 \
    -sTOTAL_STACK=1048576 -sENVIRONMENT=web,node -sMODULARIZE=0 \
    -sEXPORTED_FUNCTIONS=_main,_web_shoot -sEXPORTED_RUNTIME_METHODS=HEAPU8
ls -la "$OUT/iride.js" "$OUT/iride.wasm"
