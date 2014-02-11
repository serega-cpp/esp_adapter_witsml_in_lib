CXX=g++
CXXFLAGS=-std=c++0x -O2 -fPIC

all: pugixml

pugixml: pugixml.o
	@echo pugixml is ready.

pugixml.o: pugixml.cpp
	$(CXX) $(CXXFLAGS) -c pugixml.cpp

clean:
	rm -rf *.o

