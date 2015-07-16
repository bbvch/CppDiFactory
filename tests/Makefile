BUILD_DIR=../BuildDir/tests

CC = g++
INC = ../include
CXXFLAGS  = -g -Wall -std=c++11 -I$(INC)

DEPENDENCIES = testCase1.h

.cpp.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

MainTest: $(BUILD_DIR)/MainTest.o
	$(CC) -o $(BUILD_DIR)/MainTest $(BUILD_DIR)/MainTest.o

$(BUILD_DIR)/MainTest.o: MainTest.cpp $(INC)/CppDiFactory.h $(DEPENDENCIES) $(BUILD_DIR)
	$(CC) $(CXXFLAGS) -I$(INC) -c MainTest.cpp -o$(BUILD_DIR)/MainTest.o

all: MainTest

clean:
	rm -rf $(BUILD_DIR)
