BUILD_DIR=BuildDir
DI_CONTAINER_TEST=$(BUILD_DIR)/

CC=g++
CFLAGS=-c -Wall -O0 -g3 -std=c++11
INC=-Iinclude

run: $(SAMPLE)
	./$(SAMPLE)

.cpp.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

tests:  *
	(cd tests; ${MAKE} all);

examples: *
	(cd examples; ${MAKE} all);

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

