#ifndef __INPUTADAPTER_H__
#define __INPUTADAPTER_H__

#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <vector>

#include <boost/thread.hpp>

#include "common/TcpServer.h"
#include "cqueue.h"

#include "adapter/GenericAdapterInterface.h"

struct InputAdapter
{
    InputAdapter();

	bool start(short int port);			//!< tell adapter to start
	void stop();						//!< commands adapter to stop

    int getColumnCount();
    void setState(int st);
    bool discoverTables();
    bool discover(std::string tableName);

	void logMessage(const char *message);

    void* connectionCallBackReference;
    void* schemaInformation;
    void* parameters;
    void* rowBuf;
    void* errorObjIdentifier;

    int64_t _badRows;
    int64_t _goodRows;
    int64_t _totalRows;

	// Parameneters
    short int _listenPort;				//!< TCP server listening port number
	char	  _csvDelimiter;			//!< csv reader filed separator character

	bool _discoveryMode;
	bool _stoppedState;					//!< used to stop adapter

	TcpServer				_tcpServ;	//!< Listening TCP server object
	cqueue <std::string>	_msgQueue;	//!< Internal Queue
	
	boost::thread _accept_thread;
	boost::thread _client_thread;

	void accept_fn();							//!< Listenig thread function
	void client_fn(TcpConnectedClient *client); //!< One client processin function
};

#endif // __INPUTADAPTER_H__
