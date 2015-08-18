EXAMPLE_BUILD_DIR=../${BUILD_DIR}/examples

INC = ../include
CXXFLAGS  = -g -Wall -std=c++11 -I$(INC)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

$(EXAMPLE_BUILD_DIR):
	mkdir -p $(EXAMPLE_BUILD_DIR)

.PHONY: all clean

#################
### Example1 ####
#################
example1: $(EXAMPLE_BUILD_DIR)/helloworld.o
	$(CXX) $(CXXFLAGS) -o $(EXAMPLE_BUILD_DIR)/helloworld $(EXAMPLE_BUILD_DIR)/helloworld.o

$(EXAMPLE_BUILD_DIR)/helloworld.o: example1/helloworld.cpp $(INC)/CppDiFactory.h $(EXAMPLE_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c example1/helloworld.cpp -o $(EXAMPLE_BUILD_DIR)/helloworld.o

#################
### Example2 ####
#################
example2: $(EXAMPLE_BUILD_DIR)/singleton.o
	$(CXX) $(CXXFLAGS) -o $(EXAMPLE_BUILD_DIR)/singleton $(EXAMPLE_BUILD_DIR)/singleton.o

$(EXAMPLE_BUILD_DIR)/singleton.o: example2/singleton.cpp $(INC)/CppDiFactory.h $(EXAMPLE_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c example2/singleton.cpp -o $(EXAMPLE_BUILD_DIR)/singleton.o

#################
### Example3 ####
#################
example3: $(EXAMPLE_BUILD_DIR)/mapToExistingInstance.o
	$(CXX) $(CXXFLAGS) -o $(EXAMPLE_BUILD_DIR)/mapToExistingInstance $(EXAMPLE_BUILD_DIR)/mapToExistingInstance.o

$(EXAMPLE_BUILD_DIR)/mapToExistingInstance.o: example3/mapToExistingInstance.cpp $(INC)/CppDiFactory.h $(EXAMPLE_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c example3/mapToExistingInstance.cpp -o $(EXAMPLE_BUILD_DIR)/mapToExistingInstance.o

#################
### Example4 ####
#################
example4: $(EXAMPLE_BUILD_DIR)/classWithDependencies.o
	$(CXX) $(CXXFLAGS) -o $(EXAMPLE_BUILD_DIR)/classWithDependencies $(EXAMPLE_BUILD_DIR)/classWithDependencies.o

$(EXAMPLE_BUILD_DIR)/classWithDependencies.o: example4/classWithDependencies.cpp $(INC)/CppDiFactory.h $(EXAMPLE_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c example4/classWithDependencies.cpp -o $(EXAMPLE_BUILD_DIR)/classWithDependencies.o

all: $(EXAMPLE_BUILD_DIR) example1 example2 example3 example4


clean:
	rm -rf $(EXAMPLE_BUILD_DIR)
