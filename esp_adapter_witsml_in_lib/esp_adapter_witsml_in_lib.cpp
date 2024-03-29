#define _CRT_SECURE_NO_WARNINGS
#include "InputAdapter.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "common/Utils.h"
#include "log_message.h"

#ifdef __GNUG__
__attribute__ ((__constructor__)) void EntryPoint_linux(void) 
{
	log().apply_settings();	
	log().log_message("DllMain", Log::Info, "Adapter attached to process");
}
#endif // __GNUG__

///////////////////////////////////////////////////////////////////////////////
// Adapter API
///////////////////////////////////////////////////////////////////////////////

/*
This is the first call the Server makes when creating
an adapter. The function returns a unique
handle that the Server uses to make subsquent
calls to the adapter.
*/
extern "C" DLLEXPORT
void* createAdapter()
{
	log().log_message("DllExport", Log::Info, "exported API::createAdapter() is called");

	return new InputAdapter();
}

extern "C" DLLEXPORT
void deleteAdapter(void* adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	log().log_message("DllExport", Log::Info, "exported API::deleteAdapter() is called");

	delete inputAdapterObject;
}

/*
The Server calls this API to give the adapter implementation
a unique handle that corresponds to the connectionCallBack object on the Server
side. Use this handle as a parameter when making callbacks to the Server.
*/
extern "C" DLLEXPORT
void setCallBackReference(void *adapter,void *connectionCallBackReference)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	log().log_message("DllExport", Log::Info, "exported API::setCallBackReference() is called");

	inputAdapterObject->connectionCallBackReference = connectionCallBackReference;
}

/*
The Server calls this API to provide the adapter
implementation with information related to schema.
*/
extern "C" DLLEXPORT
void setConnectionRowType(void *adapter, void *connectionRowType)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	inputAdapterObject->schemaInformation = connectionRowType;
}

/*
The Server calls this API to provide the adapter
implementation with information related to connection
parameters.
*/
extern "C" DLLEXPORT
void setConnectionParams(void* adapter,void* connectionParams)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	inputAdapterObject->parameters = connectionParams;
}

/*
This API reads data and returns a pointer to the
data in a format the server understands. The adapter shared 
utility library provides data conversion functions.
*/
extern "C" DLLEXPORT
void* getNext(void *adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;

	// Process the Stop signal
	if (inputAdapterObject->_stoppedState) {
		inputAdapterObject->setState(RS_DONE);
		return NULL;
	}

	// Get the new message from internal Queue
	std::auto_ptr<std::string> s;
	if (!inputAdapterObject->_msgQueue.Pop(s)) {
		log().log_message("DllExport", Log::Info, "exported API::getNext(): waiting for message");

		if (!inputAdapterObject->_msgQueue.WaitForMsg()) {
			log().log_message("DllExport", Log::Info, "exported API::getNext(): message waiting was interrupted");
			logMessage(inputAdapterObject->connectionCallBackReference, L_INFO, "Internal message queue waiting was interrupted");

			return NULL;
		}

		if (!inputAdapterObject->_msgQueue.Pop(s)) {
			log().log_message("DllExport", Log::Error, "exported API::getNext(): message Queue::Pop() failed");
			logMessage(inputAdapterObject->connectionCallBackReference, L_ERR, "Internal message Queue::Pop() failed");

			return NULL;
		}
		
		log().log_message("DllExport", Log::Info, "exported API::getNext(): waiting for message completed");
	}
	
	// Split message (csv string into separate items)
	int cols_cnt = inputAdapterObject->getColumnCount();
	std::vector<std::string> fields;
	fields.reserve(cols_cnt);
	Utils::SplitCsv(*s.get(), inputAdapterObject->_csvDelimiter, fields);
	if (cols_cnt != fields.size()) {
		log().log_message("DllExport", Log::Error, "exported API::getNext(): wrong columns count in prepared row (esp_expect:%d, but csv_has:%d)", cols_cnt, fields.size());
		logMessage(inputAdapterObject->connectionCallBackReference, L_ERR, "Wrong columns count in prepared row");

		return NULL;
	}

	// Set data for each column
	for (int col_idx = 0; col_idx < cols_cnt; col_idx++) {
		::setFieldAsStringWithIndex(inputAdapterObject->rowBuf, col_idx, fields[col_idx].c_str());
	}

	// Send data to ESP server
	inputAdapterObject->_totalRows++;
	if (StreamRow streamRow = ::toRow(inputAdapterObject->rowBuf, (size_t)inputAdapterObject->_totalRows, inputAdapterObject->errorObjIdentifier)) {
		inputAdapterObject->_goodRows++;
		return streamRow;
	}

	// Send error occured
	inputAdapterObject->_badRows++;
	log().log_message("DllExport", Log::Error, "exported API::getNext(): adapter library rejected prepared row");
	logMessage(inputAdapterObject->connectionCallBackReference, L_ERR, "Adapter library rejected prepared row due to unknown reason");

	return NULL;
}

/*
This is the first life cycle API. Call this API to initialize adapter.
*/
extern "C" DLLEXPORT
bool reset(void *adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	log().log_message("DllExport", Log::Info, "exported API::reset() is called");

	inputAdapterObject->readSettings();

	if(inputAdapterObject->rowBuf)
		deleteConnectionRow(inputAdapterObject->rowBuf);

	const char *type = "RowByOrder";
	inputAdapterObject->rowBuf = ::createConnectionRow(type);
	::setStreamType(inputAdapterObject->rowBuf, inputAdapterObject->schemaInformation, false);

	inputAdapterObject->errorObjIdentifier =::createConnectionErrors();
	inputAdapterObject->setState(RS_CONTINUOUS);

	return true;
}

extern "C" DLLEXPORT
int64_t getTotalRowsProcessed(void *adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	return inputAdapterObject->_totalRows;
}

extern "C" DLLEXPORT
int64_t getNumberOfBadRows(void *adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	return inputAdapterObject->_badRows;
}

extern "C" DLLEXPORT
int64_t getNumberOfGoodRows(void *adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	return inputAdapterObject->_goodRows;
}

/*
The Server calls this API to get information from
the adapter implementation about whether there
were any errors during the processing of data.
*/
extern "C" DLLEXPORT
bool hasError(void *adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	return !(::empty(inputAdapterObject->errorObjIdentifier));
}

/*
The Server calls this API to get error information
from the adapter implementation.
*/
extern "C" DLLEXPORT
void getError(void *adapter, char** errorString)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	::getAdapterError(inputAdapterObject->errorObjIdentifier, errorString);
}

/*
Call this API immediately after reset and use it for
data processing specific to the adapter implementation.
*/
extern "C" DLLEXPORT
void start(void* adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;

	log().log_message("DllExport", Log::Info, "exported API::start() is called");
	logMessage(inputAdapterObject->connectionCallBackReference, L_INFO, log().is_enabled() ? "Adapter has started (INTERNAL_LOG:ON)" : "Adapter has started (INTERNAL_LOG:OFF)");

	inputAdapterObject->start();
}

/*
Call this API to stop an adapter.
*/
extern "C" DLLEXPORT
void stop(void* adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;

	log().log_message("DllExport", Log::Info, "exported API::stop() is called");
	logMessage(inputAdapterObject->connectionCallBackReference, L_INFO, "Adapter has stopped");

	inputAdapterObject->stop();
}

/*
Call this API to perform clean-up activities after
an adapter is stopped
*/
extern "C" DLLEXPORT
void cleanup(void* adapter)
{
	log().log_message("DllExport", Log::Info, "exported API::cleanup() is called");
}

/*
During discovery, this is the first method called by the
adapter framework to check whether an adapter supports
discovery. Adapters that support discovery return a value of true.
*/
extern "C" DLLEXPORT
bool canDiscover(void* adapter) 
{
	return true;
}

/*
This method is a pointer to an array of strings that populates
tables. The method contains table names when discovering
tables in a database, or file names when discovering particular
types of files in a directory.
*/
extern "C" DLLEXPORT
int getTableNames(void* adapter, char*** tables)
{
	return 0;
}

/*
This method returns field names.
*/
extern "C" DLLEXPORT
int getFieldNames(void* adapter, char*** names, const char* tableName)
{
	return 0;
}

/*
This method returns field types.
*/
extern "C" DLLEXPORT
int getFieldTypes(void* adapter, char*** types, const char* tableName)
{
	return 0;
}

/*
This method returns sample rows.
*/
extern "C" DLLEXPORT
int getSampleRow(void* adapter, char*** row, const char* tableName, int pos)
{
	return 0;
}

/*
This method tells the adapter that it is running in discovery mode.
*/
extern "C" DLLEXPORT
void setDiscovery(void* adapter)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	inputAdapterObject->_discoveryMode = true;
}

/*
The Server calls this API to retrieve custom statistics
information from an adapter. The Server
uses this to periodically update the _esp_adapter_
statistics metadata stream. You must enable
the time-granularity project option to update the
_esp_adapter_statistics metadata stream.

The adapter stores its statistics in key value format
within the AdapterStatistics object. The
AdapterStatistics object is populated using the
void addAdapterStatistics(void* adapterStatistics,
const char* key, const char* value) API,
which is available in the adapter utility library.
*/
extern "C" DLLEXPORT
void getStatistics(void* adapter, AdapterStatistics* adapterStatistics)
{
	InputAdapter *inputAdapterObject = (InputAdapter*)adapter;

	const char* key;
	std::ostringstream value;
	value.str("");
	key = "Total number of rows";
	value << inputAdapterObject->_totalRows;
	addAdapterStatistics(adapterStatistics, key, value.str().c_str());
	value.str("");
	key = "Total number of good rows";
	value << inputAdapterObject->_goodRows;
	addAdapterStatistics(adapterStatistics, key, value.str().c_str());
	value.str("");
	key = "Total number of bad rows";
	value << inputAdapterObject->_badRows;
	addAdapterStatistics(adapterStatistics, key, value.str().c_str());
}

/*
The Server calls this API to retrieve latency information
from the adapter, in microseconds, and
uses this information to periodically update the
latency column in the _esp_connectors metadata
stream. You must enable the time-granularity
project option to update the _esp_connectors
metadata stream.
*/
extern "C" DLLEXPORT
int64_t getLatency(void* adapter)
{
	return 100;
}

extern "C" DLLEXPORT
void commitTransaction(void *adapter)
{
}
