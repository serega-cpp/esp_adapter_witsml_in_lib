CXX=g++
CXXFLAGS=-g -O2 -I ../
OBJECTS=witsmler.o BNvalues.o SampleData.o

LIBS=-lstdc++

all: witsmler

witsmler: $(OBJECTS)
	$(MAKE) -C ../common 
	$(MAKE) -C ../pugixml
	$(CXX) -o ../witsmler.a $(CXXFLAGS) $(LIBS) $(OBJECTS) ../common/TcpClient.o ../common/Utils.o ../pugixml/pugixml.o

witsmler.o: witsmler.cpp
BNvalues.o: BNvalues.cpp
SampleData.o: SampleData.cpp
TcpClient.o: TcpClient.cpp
Utils.o: Utils.cpp

clean:
	$(MAKE) -C ../common clean 
	$(MAKE) -C ../pugixml clean 
	rm -f ../witsmler.a $(OBJECTS)
