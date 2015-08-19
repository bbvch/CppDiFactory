TEST_BUILD_DIR=../${BUILD_DIR}/tests

INC = ../include
CXXFLAGS  = -g -Wall -std=c++11 -I$(INC)

DEPENDENCIES = testCase1.h

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

$(TEST_BUILD_DIR):
	mkdir -p $(TEST_BUILD_DIR)

MainTest: $(TEST_BUILD_DIR)/MainTest.o
	$(CXX) -o $(TEST_BUILD_DIR)/MainTest $(TEST_BUILD_DIR)/MainTest.o

$(TEST_BUILD_DIR)/MainTest.o: MainTest.cpp $(INC)/CppDiFactory.h $(DEPENDENCIES) $(TEST_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC) -c MainTest.cpp -o$(TEST_BUILD_DIR)/MainTest.o

all: MainTest

clean:
	rm -rf $(TEST_BUILD_DIR)
