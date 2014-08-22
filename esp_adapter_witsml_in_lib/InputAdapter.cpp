#include "InputAdapter.h"

#include "TextMessageBuffer.h"
#include "log_message.h"

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
	_csvDelimiter(';'),
	_logMessageBody(false),
	_discoveryMode(false),
	_stoppedState(false),
	_msgQueue(64 * 1024)
{
	log().log_message("InputAdapter", Log::Debug, "Adapter Object has created");
}

bool InputAdapter::start()
{
	log().log_message("InputAdapter", Log::Info, "Starting on %d port", _listenPort);
	_stoppedState = false;

	if (!_tcpServ.Create(0, _listenPort)) {
		log().log_message("InputAdapter", Log::Error, "Failed to create listen socket");
		return false;
	}

	// initialize key generator with listen port value
	// (so different adapters will have different keys)
	GetKeyGen(_listenPort);

	_accept_thread = boost::thread(&InputAdapter::accept_fn, this);
	return true;
}

void InputAdapter::stop()
{
	log().log_message("InputAdapter", Log::Info, "Adapter has stopped");
	logMessage(connectionCallBackReference, L_INFO, "Adapter has stopped");

	_stoppedState = true;

	_accept_thread.detach();
	_tcpServ.StopServer();

	_msgQueue.Clear();
}

int InputAdapter::getColumnCount()
{
	return ::getColumnCount(schemaInformation);
}

void InputAdapter::setState(int st)
{
	::setAdapterState(connectionCallBackReference, st);
}

bool InputAdapter::discoverTables()
{
	return true;
}

bool InputAdapter::discover(std::string tableName)
{
	return true;
}

void InputAdapter::readSettings()
{
	// Get parameters from ESP project
	_listenPort = (short int)::getConnectionParamInt64_t(parameters,"ListenPort");
	_logMessageBody = ::getConnectionParamInt64_t(parameters, "LogMessageBodyEnable") != 0;
	_csvDelimiter = ';';

	std::string witsml_rules_fname = ::getConnectionParamString(parameters,"WitsmlRulesFileName");
	
	std::string witsml_rule_str;
	if (!Utils::GetFileContent(witsml_rules_fname.c_str(), witsml_rule_str)) {
		log().log_message("InputAdapter", Log::Error, "Failed open file with Witsml Rules [%s]", witsml_rules_fname.c_str());
		logMessage(connectionCallBackReference, L_ERR, "Failed open file with Witsml Rules");
	}

	if (!process_witsml_rule(witsml_rule_str, _witsmlRule)) {
		log().log_message("InputAdapter", Log::Error, "Witsml Rule file processing failed [%s]", witsml_rules_fname.c_str());
		logMessage(connectionCallBackReference, L_ERR, "Witsml Rule file processing failed");
	}
}

// Thread Function which is listening SOCKET for
// incoming connections from Scader like clients
//
void InputAdapter::accept_fn()
{
	log().log_message("InputAdapter", Log::Info, "Incoming connection acceptor has started");

	// Keep all clients here to cleanup on exit
	std::vector<std::pair<TcpConnectedClient *, boost::thread *> > clients;

	// Waiting for incoming connections from Scader like clients
	while (_tcpServ.WaitForClient()) {

		TcpConnectedClient *client = _tcpServ.GetConnectedClient(true);
		log().log_message("InputAdapter", Log::Info, "New incoming connection has accepted");

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

	log().log_message("InputAdapter", Log::Info, "Incoming connection acceptor has finished");
}

// Thread Function which is communicationg with one client
//
void InputAdapter::client_fn(TcpConnectedClient *client)
{
	log().log_message("InputAdapter", Log::Info, "New client processor has started");

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

		if (_logMessageBody)
			log().log_message("InputAdapter", Log::Debug, "<MSG-CHUNK>%s</MSG-CHUNK>", buf);

		xml_buffer.AddString(buf, received);

		std::string xml_msg = xml_buffer.GetNextMessage();
		std::vector<std::string> witsml_rows;

		for (; !xml_msg.empty(); xml_msg = xml_buffer.GetNextMessage()) {
			witsml_rows.clear();

			if (!process_witsml(xml_msg, _witsmlRule, ';', witsml_rows)) {
				log().log_message("InputAdapter", Log::Error, "Failed to parse incoming WitsML");
				logMessage(connectionCallBackReference, L_ERR, "Failed to parse incoming WitsML");
				
				continue;
			}

			for (size_t i = 0; i < witsml_rows.size(); i++)
				_msgQueue.Push(new std::string(witsml_rows[i]));
		}
	}

	log().log_message("InputAdapter", Log::Info, "New client processor has finished");
}
