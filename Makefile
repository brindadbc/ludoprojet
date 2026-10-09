# Makefile - LudoProjet
#   make            -> jeu graphique (ludo_gui)      [necessite raylib]
#   make console    -> jeu en terminal (ludo)
#   make test       -> tests automatiques
CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -O2 -Isrc -MMD -MP
SRC_DIR = src
OBJ_DIR = build

# --- code commun (moteur de jeu) ---
SRC     = $(wildcard $(SRC_DIR)/*.c)
OBJ     = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
OBJ_LIB = $(filter-out $(OBJ_DIR)/main.o,$(OBJ))

# --- interface graphique ---
GUI_SRC = $(wildcard gui/*.c)
GUI_OBJ = $(patsubst gui/%.c,$(OBJ_DIR)/gui_%.o,$(GUI_SRC))

ifeq ($(OS),Windows_NT)
  EXE         = .exe
  RAYLIB_LIBS = $(shell pkg-config --static --libs raylib 2>/dev/null || echo -lraylib -lopengl32 -lgdi32 -lwinmm) -lm
else
  EXE         =
  RAYLIB_LIBS = $(shell pkg-config --static --libs raylib 2>/dev/null || echo -lraylib -lGL -lpthread -ldl -lrt -lX11) -lm
endif
RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib 2>/dev/null)

all: ludo_gui$(EXE)

ludo_gui$(EXE): $(OBJ_LIB) $(GUI_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(RAYLIB_LIBS)

$(OBJ_DIR)/gui_%.o: gui/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -Igui -c $< -o $@

console: ludo$(EXE)

ludo$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/test_%: tests/test_%.c $(OBJ_LIB) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(OBJ_LIB)

test: $(OBJ_DIR)/test_regles $(OBJ_DIR)/test_simulation
	./$(OBJ_DIR)/test_regles
	./$(OBJ_DIR)/test_simulation

run: ludo_gui$(EXE)
	./ludo_gui$(EXE)

clean:
	rm -rf $(OBJ_DIR) ludo ludo.exe ludo_gui ludo_gui.exe

.PHONY: all console test run clean
-include $(wildcard $(OBJ_DIR)/*.d)
