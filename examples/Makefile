BUILD_DIR=../BuildDir/examples

CC = g++
INC = ../include
CXXFLAGS  = -g -Wall -std=c++11 -I$(INC)

.cpp.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: all clean

#################
### Example1 ####
#################
example1: $(BUILD_DIR)/helloworld.o
	$(CC) $(CXXFLAGS) -o $(BUILD_DIR)/helloworld $(BUILD_DIR)/helloworld.o

$(BUILD_DIR)/helloworld.o: example1/helloworld.cpp $(INC)/CppDiFactory.h $(BUILD_DIR)
	$(CC) $(CXXFLAGS) -c example1/helloworld.cpp -o $(BUILD_DIR)/helloworld.o

#################
### Example2 ####
#################
example2: $(BUILD_DIR)/singleton.o
	$(CC) $(CXXFLAGS) -o $(BUILD_DIR)/singleton $(BUILD_DIR)/singleton.o

$(BUILD_DIR)/singleton.o: example2/singleton.cpp $(INC)/CppDiFactory.h $(BUILD_DIR)
	$(CC) $(CXXFLAGS) -c example2/singleton.cpp -o $(BUILD_DIR)/singleton.o
	
#################
### Example3 ####
#################
example3: $(BUILD_DIR)/mapToExistingInstance.o
	$(CC) $(CXXFLAGS) -o $(BUILD_DIR)/mapToExistingInstance $(BUILD_DIR)/mapToExistingInstance.o

$(BUILD_DIR)/mapToExistingInstance.o: example3/mapToExistingInstance.cpp $(INC)/CppDiFactory.h $(BUILD_DIR)
	$(CC) $(CXXFLAGS) -c example3/mapToExistingInstance.cpp -o $(BUILD_DIR)/mapToExistingInstance.o

#################
### Example4 ####
#################
example4: $(BUILD_DIR)/classWithDependencies.o
	$(CC) $(CXXFLAGS) -o $(BUILD_DIR)/classWithDependencies $(BUILD_DIR)/classWithDependencies.o
	
$(BUILD_DIR)/classWithDependencies.o: example4/classWithDependencies.cpp $(INC)/CppDiFactory.h $(BUILD_DIR)
	$(CC) $(CXXFLAGS) -c example4/classWithDependencies.cpp -o $(BUILD_DIR)/classWithDependencies.o

all: $(BUILD_DIR) example1 example2 example3 example4


clean:
	rm -rf $(BUILD_DIR)
