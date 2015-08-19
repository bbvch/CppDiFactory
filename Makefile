BUILD_DIR=BuildDir
DI_CONTAINER_TEST=$(BUILD_DIR)/

CXXFLAGS=-c -Wall -O0 -g3 -std=c++11
INC=-Iinclude

.PHONY: examples tests clean run

run: $(SAMPLE)
	./$(SAMPLE)

.cpp.o:
	$(CXX) $(CFLAGS) $(INC) $< -o $@

tests:
	(cd tests; ${MAKE} all);

examples:
	(cd examples; ${MAKE} all);

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

