#!/usr/bin/env bash
# Compiles src/main.cpp against Emscripten's SDL2 port and assembles the demo site
# in web/dist. Needs em++ on PATH (https://emscripten.org). CI runs this for Pages.
set -euo pipefail
cd "$(dirname "$0")"
rm -rf dist && mkdir -p dist
em++ -std=c++17 -O3 -Wall -Wextra -I../include ../src/main.cpp \
  -sUSE_SDL=2 -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS=_main,_set_culling,_set_depth_view,_set_paused,_get_frame_ms,_get_triangles_drawn \
  -o dist/forge_engine.js
cp index.html app.js demo.css dist/
echo "built web/dist"
