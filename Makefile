PLUGIN_NAME = plugin.so
GCC_PLUGIN_DIR = $(shell g++ --print-file-name=plugin)
CXX = g++
CXXFLAGS = -I$(GCC_PLUGIN_DIR)/include -fPIC -shared -std=c++23 -O0

plugin: plugin.cc
	$(CXX) $(CXXFLAGS) plugin.cc -o $(PLUGIN_NAME)

test: plugin test.cc
	$(CXX) -fplugin=./$(PLUGIN_NAME) test.cc -o test.out

all: test

clean:
	rm -f $(PLUGIN_NAME) *.out