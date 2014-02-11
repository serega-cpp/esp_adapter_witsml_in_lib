CXX=g++
CXXFLAGS=-g -O2 -fPIC
OBJECTS=tcp_sock.o TcpClient.o TcpServer.o Utils.o

all: common_objs

common_objs: $(OBJECTS)

# $(CXX) $(CXXFLAGS) -c $(OBJECTS)

tcp_sock.o: tcp_sock.cpp
TcpClient.o: TcpClient.cpp
TcpServer.o: TcpServer.cpp
Utils.o: Utils.cpp

clean:
	rm -f *.o 
