TARGET = build/neosu
SOURCES = $(shell find src -type f -name '*.cpp')
OBJECTS = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))
HEADERS = $(shell find src -type f -name '*.h')

LIBS = blkid freetype2 glew libenet libjpeg liblzma xi zlib

CXXFLAGS = -std=c++17 -fmessage-length=0 -fno-exceptions -Wno-sign-compare -Wno-unused-local-typedefs -Wno-reorder -Wno-switch -Wall
CXXFLAGS += `pkgconf --static --cflags $(LIBS)` `curl-config --cflags`
CXXFLAGS += -Isrc/App -Isrc/App/Osu -Isrc/Engine -Isrc/GUI -Isrc/GUI/Windows -Isrc/GUI/Windows/VinylScratcher -Isrc/Engine/Input -Isrc/Engine/Platform -Isrc/Engine/Main -Isrc/Engine/Renderer -Isrc/Util
CXXFLAGS += -Ilibraries/bass/include -Ilibraries/bassasio/include -Ilibraries/bassfx/include -Ilibraries/bassmix/include -Ilibraries/basswasapi/include -Ilibraries/bassloud/include
CXXFLAGS += -O2 -g3 -fsanitize=address

LDFLAGS = -ldiscord-rpc -lbass -lbassmix -lbass_fx -lbassloud -lstdc++
LDFLAGS += `pkgconf --static --libs $(LIBS)` `curl-config --static-libs --libs`
LDFLAGS += -Lrosu-pp-c/target/release -lrosu_pp_c


$(TARGET): build $(OBJECTS) $(HEADERS) rosu-pp-c/target/release/librosu_pp_c.a
	@echo "CC" $(TARGET)
	@$(CXX) -o $(TARGET) $(OBJECTS) $(CXXFLAGS) $(LDFLAGS)

obj/%.o: src/%.cpp
	@echo "CC" $<
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

rosu-pp-c/target/release/librosu_pp_c.a:
	@mkdir -p obj
	cd rosu-pp-c && cargo build --release

build:
	cp -nr resources build

.PHONY: clean
clean:
	rm -rf build obj
