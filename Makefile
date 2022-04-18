CXX=g++
CXXFLAGS=-std=c++17 -O3 -Wall -Wextra
GSTFLAGS1=`pkg-config --cflags gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0`
GSTFLAGS2=`pkg-config --libs gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0`

.PHONY: clean, all, build

all: build build/ptz-demo-client.out build/ptz-demo-server.out

clean:
	rm -rf build/*

build:
	mkdir -p build 

build/ptz-demo-client.out: build/ptz-demo-client.o
	$(CXX) $(CXXFLAGS) $< -o $@ $(GSTFLAGS2)

build/ptz-demo-client.o: ptz-demo-client.cpp ptz-demo-client.hpp
	$(CXX) $(CXXFLAGS) $(GSTFLAGS1) $< -c -o $@

build/ptz-demo-server.out: build/ptz-demo-server.o
	$(CXX) $(CXXFLAGS) $< -o $@ $(GSTFLAGS2)

build/ptz-demo-server.o: ptz-demo-server.cpp ptz-demo-server.hpp src/server-helpers.hpp
	$(CXX) $(CXXFLAGS) $(GSTFLAGS1) $< -c -o $@
