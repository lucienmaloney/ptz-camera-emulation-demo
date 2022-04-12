CXX=g++
CXXFLAGS=-std=c++17 -O3 -Wall -Wextra -I include/
#gstreamer/=`pkg-config --cflags --libs gstreamer-1.0` -I/usr/include/gstreamer-1.0

.PHONY: clean, all, build

all: build build/ptz-demo-client.out build/ptz-demo-server.out

clean:
	rm -rf build/*

build:
	mkdir -p build 

build/ptz-demo-client.out: ptz-demo-client.cpp ptz-demo-client.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

build/ptz-demo-server.out: ptz-demo-server.cpp ptz-demo-server.hpp
	$(CXX) $(CXXFLAGS) $< -o $@
