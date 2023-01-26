
CONAN_DIR := conan
BUILD_DIR := build
SOURCE_DIR := src

.PHONY: all lib test clean

all: lib

$(CONAN_DIR):
	conan install -if $(CONAN_DIR) .

$(BUILD_DIR): $(CONAN_DIR)
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. -DCMAKE_TOOLCHAIN_FILE=conan/conan_toolchain.cmake -DCMAKE_BUILD_TYPE="Release"

$(SOURCE_DIR): $(BUILD_DIR)
	make -C $(BUILD_DIR)

lib: $(SOURCE_DIR)

test: lib
	@./$(BUILD_DIR)/test/test

clean:
	@rm -f CMakeUserPresets.json
	@rm -rf $(BUILD_DIR)
