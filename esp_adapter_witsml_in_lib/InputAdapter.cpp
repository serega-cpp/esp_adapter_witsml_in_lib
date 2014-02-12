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

				// get UID from the hard-coded position
				assert(!tables.empty());
				std::string uid(tables[0].at(0).second);

				// used only to meet ESP requirenments for 
				// obligatory PRIMARY KEY on Input Windows,
				// so PRIMARY KEY is (uid; uid_salt)
				unsigned int uid_salt = 1;

				std::string log_NameWell;
				std::string log_NameWellbore;
				std::string logHeader_UomNamingSystem;
				std::string commonData_NameSource;
				std::vector<LogCurveInfoRec> logCurveInfo;
				std::vector<std::string> logData;

				for (std::vector<TableData>::iterator table_it = tables.begin(); table_it != tables.end(); ++table_it) {
					std::string group(table_it->GetName());
					logMessage(group.c_str());

					// array variables
					if (Utils::strcmpi(group.c_str(), "logCurveInfo") == 0) {	// columns

						unsigned int columnIndex;
						std::string mnemonic;
						std::string	mnemAlias;
						std::string curveDescription;
						std::string	startIndex;
						std::string	endIndex;

						for (TableData::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {
							if (Utils::strcmpi(row_it->first.c_str(), "columnIndex") == 0) columnIndex = atoi(row_it->second.c_str());
							else if (Utils::strcmpi(row_it->first.c_str(), "startIndex") == 0) startIndex.assign(row_it->second);
							else if (Utils::strcmpi(row_it->first.c_str(), "endIndex") == 0) endIndex.assign(row_it->second);
							else if (Utils::strcmpi(row_it->first.c_str(), "mnemAlias") == 0) mnemAlias.assign(row_it->second);
							else if (Utils::strcmpi(row_it->first.c_str(), "curveDescription") == 0) curveDescription.assign(row_it->second);
							else if (Utils::strcmpi(row_it->first.c_str(), "mnemonic") == 0) mnemonic.assign(row_it->second);
						}
						
						char ci_str[16];
						sprintf(ci_str, "_%d", columnIndex);
						std::string	hash2 = hash(mnemonic).append(ci_str);

						LogCurveInfoRec rec(hash2, columnIndex, mnemAlias.append(curveDescription), startIndex, endIndex);
						logCurveInfo.push_back(rec);
					}
					else if (Utils::strcmpi(group.c_str(), "logData") == 0) {	// rows
						for (TableData::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {
							logData.push_back(row_it->second);
						}
					}
					// scalar variables
					else if (Utils::strcmpi(group.c_str(), "log") == 0) {
						for (TableData::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {
							if (Utils::strcmpi(row_it->first.c_str(), "NameWell") == 0) log_NameWell.assign(row_it->second);
							else if (Utils::strcmpi(row_it->first.c_str(), "NameWellbore") == 0) log_NameWellbore.assign(row_it->second);
						}
					}
					else if (Utils::strcmpi(group.c_str(), "logHeader") == 0) {
						for (TableData::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {
							if (Utils::strcmpi(row_it->first.c_str(), "UomNamingSystem") == 0) logHeader_UomNamingSystem.assign(row_it->second);
						}
					}
					else if (Utils::strcmpi(group.c_str(), "commonData") == 0) {
						for (TableData::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {
							if (Utils::strcmpi(row_it->first.c_str(), "NameSource") == 0) commonData_NameSource.assign(row_it->second);
						}
					}
				}

				if (logCurveInfo.empty())
					continue;

				std::string hash1 = hash(log_NameWell, log_NameWellbore, logHeader_UomNamingSystem, commonData_NameSource);

				std::vector<std::string> fieldsData;
				for (std::vector<std::string>::const_iterator data_it = logData.begin(); data_it != logData.end(); ++data_it) {

					fieldsData.clear();
					Utils::SplitCsv(*data_it, ',', fieldsData);
					if (fieldsData.size() < logCurveInfo.size())
						continue;

					for (std::vector<LogCurveInfoRec>::const_iterator info_it = logCurveInfo.begin() + 1; info_it != logCurveInfo.end(); ++info_it) {

						std::stringstream csv_row;
						csv_row << fieldsData[0] << ";"
								<< hash1 << info_it->hash2 << ";" 
								<< fieldsData[info_it->uint_columnIndex] << ";" 
								<< info_it->uint_columnIndex << ";"
								<< info_it->text_columnIndex << ";" 
								<< info_it->startIndex << ";" 
								<< info_it->endIndex << ";" 
								<< log_NameWell << ";" 
								<< log_NameWellbore << ";" 
								<< logHeader_UomNamingSystem << ";" 
								<< commonData_NameSource << ";" 
								<< hash1 << std::endl;

						_msgQueue.Push(new std::string(csv_row.str()));
					}
				}

			} // for (root_it = root->children().begin(); root_it != root->children().end(); ++root_it)
		} // for (xml_msg = xml_buffer.GetNextMessage())
	} // while (received = client->Recv(buf))

	logMessage("client_fn completed");
}
