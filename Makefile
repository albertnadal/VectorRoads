CC_MAC = gcc

SRC = \
	src/main.c \
	src/level.c \
	src/lane.c \
	src/tunnel.c \
	src/explosion.c

SDKROOT = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.5.sdk

CFLAGS_MAC = -std=c11 -Ofast -march=native -flto \
             -fno-signed-zeros -fno-trapping-math -funroll-loops \
             -Wno-deprecated \
             -I/usr/local/include -I. -Isrc -Ithird_party \
             -isysroot $(SDKROOT)

LDFLAGS_MAC = -Wl,-search_paths_first \
              -Wl,-headerpad_max_install_names \
              -framework OpenGL \
              -framework Cocoa \
              -framework IOKit \
              -framework CoreAudio \
              -framework CoreVideo \
              -framework CoreFoundation \
              -Lthird_party/raylib -lraylib \
              -Lthird_party/box3d -lbox3d

EXEC_MAC = main

OBJ_MAC = $(SRC:src/%.c=build/macos/%.o)

CC_WEB = emcc

CFLAGS_WEB = -std=c11 -Ofast -flto \
             -fno-signed-zeros -fno-trapping-math -funroll-loops \
             -Wno-deprecated \
             -I. -Isrc -Ithird_party \
             -DPLATFORM_WEB

LDFLAGS_WEB = -Lthird_party/raylib -lraylib.web \
              -Lthird_party/box3d -lbox3d.web \
              -sUSE_GLFW=3 \
              -sASYNCIFY \
              --preload-file audio \
              --preload-file fonts \
              --preload-file images \
              --preload-file levels \
              --preload-file models

EXEC_WEB = index.html

OBJ_WEB = $(SRC:src/%.c=build/web/%.o)

CC_LINUX = gcc

CFLAGS_LINUX = -std=c11 -Ofast -march=native -flto \
             -fno-signed-zeros -fno-trapping-math -funroll-loops \
             -Wno-deprecated \
             -I/usr/local/include -I. -Isrc -Ithird_party

LDFLAGS_LINUX = -Lthird_party/raylib -lraylib.linux \
				-Lthird_party/box3d -lbox3d.linux \
				-lm

EXEC_LINUX = main_linux

OBJ_LINUX = $(SRC:src/%.c=build/linux/%.o)

.PHONY: default
default: mac

.PHONY: mac
mac: $(EXEC_MAC)

$(EXEC_MAC): $(OBJ_MAC)
	$(CC_MAC) $(CFLAGS_MAC) $(OBJ_MAC) -o $(EXEC_MAC) $(LDFLAGS_MAC)

build/macos/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC_MAC) $(CFLAGS_MAC) -c $< -o $@

.PHONY: web
web: $(EXEC_WEB)

$(EXEC_WEB): $(OBJ_WEB)
	$(CC_WEB) $(CFLAGS_WEB) $(OBJ_WEB) -o $(EXEC_WEB) $(LDFLAGS_WEB)

build/web/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC_WEB) $(CFLAGS_WEB) -c $< -o $@

.PHONY: linux
linux: $(EXEC_LINUX)

$(EXEC_LINUX): $(OBJ_LINUX)
	$(CC_LINUX) $(CFLAGS_LINUX) $(OBJ_LINUX) -o $(EXEC_LINUX) $(LDFLAGS_LINUX)

build/linux/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC_LINUX) $(CFLAGS_LINUX) -c $< -o $@

.PHONY: all
all: mac web

.PHONY: clean
clean:
	rm -f $(EXEC_MAC)
	rm -f $(EXEC_WEB) index.js index.wasm index.data
	rm -f $(EXEC_LINUX)
	rm -rf build
