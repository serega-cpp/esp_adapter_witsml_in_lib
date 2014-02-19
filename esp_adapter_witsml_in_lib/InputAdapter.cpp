#include "InputAdapter.h"

#include "TextMessageBuffer.h"
#include "WitsmlProcessor.h"
#include "log_message.h"

InputAdapter::InputAdapter(char csvDelimiter): 
	connectionCallBackReference(0),
	schemaInformation(0),
	parameters(0),
	rowBuf(0),
	errorObjIdentifier(0),
    _badRows(0),
    _goodRows(0),
    _totalRows(0),
    _listenPort(12345),
	_csvDelimiter(csvDelimiter),
	_discoveryMode(false),
	_stoppedState(false),
	_msgQueue(64 * 1024)
{
	log_message("InputAdapter", "[ctor]");
}

bool InputAdapter::start(short int port)
{
	log_message("InputAdapter", "start on %d port", port);
	_stoppedState = false;

	if (!_tcpServ.Create(0, port)) {
		log_message("InputAdapter", "<ERROR> Failed to create TCP SERVER");
		return false;
	}

	_accept_thread = boost::thread(&InputAdapter::accept_fn, this);
	return true;
}

void InputAdapter::stop()
{
	log_message("InputAdapter", "stop");
	_stoppedState = true;

	_accept_thread.detach();
	_tcpServ.StopServer();

	_msgQueue.Clear();
}

int InputAdapter::getColumnCount()
{
	// log_message("InputAdapter", "getColumnCount");
    return ::getColumnCount(schemaInformation);
}

void InputAdapter::setState(int st)
{
	// log_message("InputAdapter", "setState");
    ::setAdapterState(connectionCallBackReference, st);
}

bool InputAdapter::discoverTables()
{
	log_message("InputAdapter", "discoverTables");
    return true;
}

bool InputAdapter::discover(std::string tableName)
{
	log_message("InputAdapter", "discover");
    return true;
}

// Thread Function which is listening SOCKET for
// incoming connections from Scader like clients
//
void InputAdapter::accept_fn()
{
	log_message("InputAdapter", "accept_fn enetring");

	// Keep all clients here to cleanup on exit
	std::vector<std::pair<TcpConnectedClient *, boost::thread *> > clients;

	// Waiting for incoming connections from Scader like clients
	while (_tcpServ.WaitForClient()) {

		TcpConnectedClient *client = _tcpServ.GetConnectedClient(true);
		log_message("InputAdapter", "new client connection");

		// Start separate thread for each client
		// (can be replaced by 'select' like architecture in future)
		clients.push_back(std::pair<TcpConnectedClient *, boost::thread *>(client, new boost::thread(&InputAdapter::client_fn, this, client)));
	}

	// Wake up, close and destroy clients
	for (std::vector<std::pair<TcpConnectedClient *, boost::thread *> >::iterator it = clients.begin(); it != clients.end(); ++it) {
		(it->second)->detach();
		(it->first)->Close();
		delete it->first;
		delete it->second;
	}

	log_message("InputAdapter", "accept_fn completed");
}

// Thread Function which is communicationg with one client
//
void InputAdapter::client_fn(TcpConnectedClient *client)
{
	log_message("InputAdapter", "client_fn entering");

	int received;
	char buf[8192];

	TextMessageBuffer	xml_buffer("&&");
	
	// Recieve message (string ended by '\n') from client and
	// push it into internal Queue (currently combined messages
	// like 'msg1\n msg2\n msg3\n' not processed properly - 
	// messages 2 & 3 will be discarded)
	//
	while ((received = client->Recv(buf, sizeof(buf) - 1)) > 0) {
		buf[received] = '\0';

		xml_buffer.AddString(buf, received);

		std::string xml_msg = xml_buffer.GetNextMessage();
		std::vector<std::string> witsml_rows;

		for (; !xml_msg.empty(); xml_msg = xml_buffer.GetNextMessage()) {
			witsml_rows.clear();

			if (!process_witsml(xml_msg, ';', witsml_rows)) {
				log_message("InputAdapter", "<ERROR> Failed to parse XML");
				continue;
			}

			for (size_t i = 0; i < witsml_rows.size(); i++)
				_msgQueue.Push(new std::string(witsml_rows[i]));
		}
	}

	log_message("InputAdapter", "client_fn completed");
}
