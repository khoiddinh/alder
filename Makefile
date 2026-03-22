BUILD := build

.PHONY: all build clean test rebuild

all: build

build:
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD)

debug:
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD)

test: build
	cd $(BUILD) && ctest --output-on-failure

clean:
	rm -rf $(BUILD)

rebuild: clean build
