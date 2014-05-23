CXX=g++
CXXFLAGS=-I$(ESP_HOME)include/ -I../boost -I../ -g -O2 -fPIC
LDFLAGS=-L$(ESP_HOME)lib/adapters/ -L$(ESP_HOME)lib/
OBJECTS=cqueue.o esp_adapter_witsml_in_lib.o InputAdapter.o TextMessageBuffer.o WitsmlProcessor.o log_message.o

LIBS=-lstdc++ -lesp_adapter_util_lib -lesp_server_lib -lboost_thread

all: esp_adapter_witsml_in_lib

esp_adapter_witsml_in_lib: $(OBJECTS)
	$(MAKE) -C ../common 
	$(MAKE) -C ../pugixml
	$(CXX) -shared -o ../libesp_adapter_witsml_in_lib.so $(CXXFLAGS) $(LDFLAGS) $(LIBS) $(OBJECTS) ../pugixml/pugixml.o ../common/TcpServer.o ../common/Utils.o

cqueue.o: cqueue.cpp
esp_adapter_witsml_in_lib.o: esp_adapter_witsml_in_lib.cpp
InputAdapter.o: InputAdapter.cpp
TextMessageBuffer.o: TextMessageBuffer.cpp
WitsmlProcessor.o: WitsmlProcessor.cpp
log_message.o: log_message.cpp

clean:
	$(MAKE) -C ../common clean
	$(MAKE) -C ../pugixml clean 
	rm -f ../libesp_adapter_witsml_in_lib.so $(OBJECTS)
