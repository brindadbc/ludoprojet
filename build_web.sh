#!/bin/sh
# Compile le jeu en WebAssembly (navigateur) -> dossier web/
# Prerequis : Emscripten (emcc) et raylib compile pour le web (libraylib.web.a).
#   RAYLIB_SRC=/chemin/vers/raylib/src ./build_web.sh
set -e
RAYLIB_SRC="${RAYLIB_SRC:-/tmp/raylib-5.5/src}"
mkdir -p web
emcc -O2 -std=gnu99 -Isrc -Igui -I"$RAYLIB_SRC" -DPLATFORM_WEB \
  src/affichage.c src/file.c src/historique.c src/joueur.c src/plateau.c \
  src/regles.c src/sauvegarde.c src/utils.c \
  gui/main_gui.c gui/gui_dessin.c gui/gui_jeu.c gui/gui_menu.c web_src/glfw_compat.c \
  "$RAYLIB_SRC/libraylib.web.a" \
  -o web/index.html --shell-file web_src/shell.html \
  --preload-file fonts_web/fonts@/fonts \
  -sMINIFY_HTML=0 -sUSE_GLFW=3 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 \
  -sFORCE_FILESYSTEM=1 -lidbfs.js -sTOTAL_STACK=1048576
echo "OK : web/index.html"
