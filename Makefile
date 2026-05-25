CC = gcc
# Use absolute paths or fix relative paths for GCC on Windows/MSYS
PROJ_DIR = $(CURDIR)
CFLAGS = -Wall -Wextra -std=c99 -O2 -I"$(PROJ_DIR)/include" \
         -I"$(PROJ_DIR)/thirdparty/SDL2-2.30.3/x86_64-w64-mingw32/include" \
         -I"$(PROJ_DIR)/thirdparty/SDL2-2.30.3/x86_64-w64-mingw32/include/SDL2" \
         -I"$(PROJ_DIR)/thirdparty/SDL2_mixer-2.8.2/x86_64-w64-mingw32/include" \
         -I"$(PROJ_DIR)/thirdparty/SDL2_mixer-2.8.2/x86_64-w64-mingw32/include/SDL2"
LDFLAGS = -L"$(PROJ_DIR)/thirdparty/SDL2-2.30.3/x86_64-w64-mingw32/lib" \
          -L"$(PROJ_DIR)/thirdparty/SDL2_mixer-2.8.2/x86_64-w64-mingw32/lib" \
          -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer

SRC = src/main.c src/vector_graphics.c src/game.c src/vector_font.c src/audio.c
OBJ = $(SRC:.c=.o)
TARGET = permadrift.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	del /q /f src\*.o src\*.wasm.o $(TARGET) $(WASM_TARGET) index.js index.wasm 2>nul || exit 0

WASM_CC = emcc
WASM_CFLAGS = -Wall -Wextra -std=c99 -O2 -I"$(PROJ_DIR)/include" -s USE_SDL=2 -s USE_SDL_MIXER=2
WASM_LDFLAGS = -s USE_SDL=2 -s USE_SDL_MIXER=2 -s ALLOW_MEMORY_GROWTH=1 \
               --shell-file "$(PROJ_DIR)/src/wasm_shell.html"
WASM_TARGET = index.html
WASM_OBJ = $(SRC:.c=.wasm.o)

wasm: $(WASM_TARGET)

$(WASM_TARGET): $(WASM_OBJ)
	$(WASM_CC) -o $@ $^ $(WASM_LDFLAGS)

%.wasm.o: %.c
	$(WASM_CC) $(WASM_CFLAGS) -c -o $@ $<
