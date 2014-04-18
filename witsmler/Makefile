CXX=g++
CXXFLAGS=-g -O2 -I ../
OBJECTS=witsmler.o

LIBS=-lstdc++

all: witsmler

witsmler: $(OBJECTS)
	$(MAKE) -C ../common 
	$(CXX) -o ../witsmler.a $(CXXFLAGS) $(LIBS) $(OBJECTS) ../common/TcpClient.o ../common/Utils.o

witsmler.o: witsmler.cpp

clean:
	$(MAKE) -C ../common clean 
	rm -f ../witsmler.a $(OBJECTS)
