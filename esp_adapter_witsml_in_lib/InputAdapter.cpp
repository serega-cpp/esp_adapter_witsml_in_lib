#include "InputAdapter.h"
#include <assert.h>
#include <sstream>

#include "pugixml/pugixml.hpp"

#include "TextMessageBuffer.h"
#include "WitsmlProcessor.h"

void InputAdapter::logMessage(const char *message)
{
	return;

#ifdef _WIN32
	const char *log_file = "c:\\temp\\esp_adapter_InputAdapter.log";
#else
	const char *log_file = "/home/serega/esp_adapter_InputAdapter.log";
#endif

	FILE *f = fopen(log_file, "at");
	fputs(message, f); 
	fputs("\n", f);
	fclose(f);
}

InputAdapter::InputAdapter(): 
	connectionCallBackReference(0),
	schemaInformation(0),
	parameters(0),
	rowBuf(0),
	errorObjIdentifier(0),
    _badRows(0),
    _goodRows(0),
    _totalRows(0),
    _listenPort(12345),
	_csvDelimiter(','),
	_discoveryMode(false),
	_stoppedState(false),
	_msgQueue(64 * 1024)
{
	logMessage("[ctor]");
}

bool InputAdapter::start(short int port)
{
	logMessage("start");
	_stoppedState = false;

	if (!_tcpServ.Create(0, port)) {
		logMessage("<ERROR> [ctor]: Failed to create TCP SERVER on localhost:12345");
		return false;
	}

	_accept_thread = boost::thread(&InputAdapter::accept_fn, this);
	return true;
}

void InputAdapter::stop()
{
	logMessage("stop");
	_stoppedState = true;

	_accept_thread.detach();
	_tcpServ.StopServer();

	_msgQueue.Clear();
}

int InputAdapter::getColumnCount()
{
	// logMessage("getColumnCount");
    return ::getColumnCount(schemaInformation);
}

void InputAdapter::setState(int st)
{
	// logMessage("setState");
    ::setAdapterState(connectionCallBackReference, st);
}

bool InputAdapter::discoverTables()
{
	logMessage("discoverTables");
    return true;
}

bool InputAdapter::discover(std::string tableName)
{
	logMessage("discover");
    return true;
}

// Thread Function which is listening SOCKET for
// incoming connections from Scader like clients
//
void InputAdapter::accept_fn()
{
	logMessage("accept_fn enetring");

	// Keep all clients here to cleanup on exit
	std::vector<std::pair<TcpConnectedClient *, boost::thread *> > clients;

	// Waiting for incoming connections from Scader like clients
	while (_tcpServ.WaitForClient()) {

		TcpConnectedClient *client = _tcpServ.GetConnectedClient(true);
		logMessage("connected client");

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

	logMessage("accept_fn completed");
}

// Thread Function which is communicationg with one client
//
void InputAdapter::client_fn(TcpConnectedClient *client)
{
	logMessage("client_fn entering");

	int received;
	char buf[8192];

	TextMessageBuffer	xml_buffer("&&");
	pugi::xml_document	xml_doc;
	
	// Recieve message (string ended by '\n') from client and
	// push it into internal Queue (currently combined messages
	// like 'msg1\n msg2\n msg3\n' not processed properly - 
	// messages 2 & 3 will be discarded)
	//
	while ((received = client->Recv(buf, sizeof(buf) - 1)) > 0) {
		buf[received] = '\0';

		xml_buffer.AddString(buf, received);

		std::string xml_msg = xml_buffer.GetNextMessage();

		for (; !xml_msg.empty(); xml_msg = xml_buffer.GetNextMessage()) {

			// parse xml
			pugi::xml_parse_result result = xml_doc.load_buffer(xml_msg.c_str(), xml_msg.length());
			if (!result) {
				logMessage("ERROR: Failed to parse XML");
				continue;
			}

			pugi::xml_node_iterator root = xml_doc.children().begin();
			for (pugi::xml_node_iterator root_it = root->children().begin(); root_it != root->children().end(); ++root_it) {
		
				std::vector<TableData> tables;
				if (!traverse_xml(root_it, std::pair<std::string, std::string>(), tables))
					continue;

				assert(!tables.empty());
				std::string uid(tables[0].at(0).second);

				// used only to meet ESP requirenments for 
				// obligatory PRIMARY KEY on Input Windows,
				// so PRIMARY KEY is (uid; uid_salt)
				unsigned int uid_salt = 1;

				for (std::vector<TableData>::iterator table_it = tables.begin(); table_it != tables.end(); ++table_it) {
					std::string group(table_it->GetName());

					logMessage(group.c_str());
					for (TableData::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {
						
						std::stringstream csv_row;
						csv_row << uid << ";" << uid_salt++ << ";" << group << ";" << row_it->first << ";" << row_it->second << std::endl;

						_msgQueue.Push(new std::string(csv_row.str()));
					}
				}
			}
		}
	}

	logMessage("client_fn completed");
}
