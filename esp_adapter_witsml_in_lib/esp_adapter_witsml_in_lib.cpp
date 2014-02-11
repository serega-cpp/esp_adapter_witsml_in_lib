#define _CRT_SECURE_NO_WARNINGS
#include "InputAdapter.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "common/Utils.h"

void LogMessage(const char *message)
{
	return;

#ifdef _WIN32
	const char *log_file = "c:\\temp\\esp_adapter_witsml_in_lib.log";
#else
	const char *log_file = "/home/serega/esp_adapter_witsml_in_lib.log";
#endif

	FILE *f = fopen(log_file, "at");
	fputs(message, f); 
	fputs("\n", f);
	fclose(f);
}

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
	LogMessage("createAdapter");

    return new InputAdapter();
}

extern "C" DLLEXPORT
void deleteAdapter(void* adapter)
{
	LogMessage("deleteAdapter");

    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
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
	LogMessage("setCallBackReference");

    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
    inputAdapterObject->connectionCallBackReference = connectionCallBackReference;
}

/*
The Server calls this API to provide the adapter
implementation with information related to schema.
*/
extern "C" DLLEXPORT
void setConnectionRowType(void *adapter,void *connectionRowType)
{
	LogMessage("setConnectionRowType");

    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
    inputAdapterObject->schemaInformation = connectionRowType;
}

/*
The Server calls this API to provide the adapter
implementation with information related to connection
parameters.
*/
extern "C" DLLEXPORT
void  setConnectionParams(void* adapter,void* connectionParams)
{
	LogMessage("setConnectionParams");

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
	LogMessage("getNext");
    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	
	// Process the Stop signal
	if (inputAdapterObject->_stoppedState) {
		inputAdapterObject->setState(RS_DONE);
		return NULL;
	}

	// Get the new message from internal Queue
	std::auto_ptr<std::string> s;
	if (!inputAdapterObject->_msgQueue.Pop(s)) {
		LogMessage("getNext::Waiting for Msg");
		if (!inputAdapterObject->_msgQueue.WaitForMsg()) {
			LogMessage("getNext::Waiting failed");
			return NULL;
		}
		if (!inputAdapterObject->_msgQueue.Pop(s)) {
			LogMessage("getNext::Pop failed");
			return NULL;
		}
	}
	
	// Split message (csv string into separate items)
	LogMessage("getNext::Waiting completed");
	int cols_cnt = inputAdapterObject->getColumnCount();
	std::vector<std::string> fields;
	fields.reserve(cols_cnt);
	Utils::SplitCsv(*s.get(), inputAdapterObject->_csvDelimiter, fields);
	if (cols_cnt != fields.size()) {
		LogMessage("getNext::Wrong columns count");
		return NULL;
	}

	// Set data for each column
	for (int col_idx = 0; col_idx < cols_cnt; col_idx++)
		::setFieldAsStringWithIndex(inputAdapterObject->rowBuf, col_idx, fields[col_idx].c_str());

	// Send data to ESP server
	inputAdapterObject->_totalRows++;
	if (StreamRow streamRow = ::toRow(inputAdapterObject->rowBuf, (size_t)inputAdapterObject->_totalRows, inputAdapterObject->errorObjIdentifier)) {
		inputAdapterObject->_goodRows++;
		return streamRow;
	}

	// Send error occured
	inputAdapterObject->_badRows++;
	return NULL;
}

/*
This is the first life cycle API. Call this API to initialize adapter.
*/
extern "C" DLLEXPORT
bool reset(void *adapter)
{
	LogMessage("reset");

    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;

	// Get parameters, but currently don't use it
    inputAdapterObject->_listenPort = (short int)::getConnectionParamInt64_t(inputAdapterObject->parameters,"ListenPort");
	inputAdapterObject->_csvDelimiter = *(::getConnectionParamString(inputAdapterObject->parameters,"CsvDelimiter"));
    
	if(inputAdapterObject->rowBuf)
        deleteConnectionRow(inputAdapterObject->rowBuf);

    std::string type = "RowByOrder";
    inputAdapterObject->rowBuf = ::createConnectionRow(type.c_str());
    ::setStreamType(inputAdapterObject->rowBuf, inputAdapterObject->schemaInformation, false);
    
	inputAdapterObject->errorObjIdentifier =::createConnectionErrors();
    inputAdapterObject->setState(RS_CONTINUOUS);
    
	return true;
}

extern "C" DLLEXPORT
int64_t getTotalRowsProcessed(void *adapter)
{
	// LogMessage("getTotalRowsProcessed");

    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
    return inputAdapterObject->_totalRows;
}

extern "C" DLLEXPORT
int64_t getNumberOfBadRows(void *adapter)
{
	// LogMessage("getNumberOfBadRows");

    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
    return inputAdapterObject->_badRows;
}

extern "C" DLLEXPORT
int64_t getNumberOfGoodRows(void *adapter)
{
	// LogMessage("getNumberOfGoodRows");

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
	// LogMessage("hasError");

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
	LogMessage("getError");

    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
    ::getAdapterError(inputAdapterObject->errorObjIdentifier, errorString);
}

/*
Call this API immediately after reset and use it for
data processing specific to the adapter implementation.
*/
extern "C" DLLEXPORT
void  start(void* adapter)
{
	LogMessage("start");
    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	inputAdapterObject->start(inputAdapterObject->_listenPort);
}

/*
Call this API to stop an adapter.
*/
extern "C" DLLEXPORT
void  stop(void* adapter)
{
	LogMessage("stop");
    InputAdapter *inputAdapterObject = (InputAdapter*)adapter;
	inputAdapterObject->stop();
}

/*
Call this API to perform clean-up activities after
an adapter is stopped
*/
extern "C" DLLEXPORT
void  cleanup(void* adapter)
{
	LogMessage("cleanup");
}

/*
During discovery, this is the first method called by the
adapter framework to check whether an adapter supports
discovery. Adapters that support discovery return a value of true.
*/
extern "C" DLLEXPORT
bool  canDiscover(void* adapter) 
{
	LogMessage("canDiscover");

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
	LogMessage("getTableNames");
	// .. content removed
	return 0;
}

/*
This method returns field names.
*/
extern "C" DLLEXPORT
int getFieldNames(void* adapter, char*** names, const char* tableName)
{
	LogMessage("getFieldNames");
	// .. content removed
	return 0;
}

/*
This method returns field types.
*/
extern "C" DLLEXPORT
int getFieldTypes(void* adapter, char*** types, const char* tableName)
{
	LogMessage("getFieldTypes");
	// .. content removed
	return 0;
}

/*
This method returns sample rows.
*/
extern "C" DLLEXPORT
int getSampleRow(void* adapter, char*** row, const char* tableName, int pos)
{
	LogMessage("getSampleRow");
	// .. content removed
	return 0;
}

/*
This method tells the adapter that it is running in discovery mode.
*/
extern "C" DLLEXPORT
void setDiscovery(void* adapter)
{
	LogMessage("setDiscovery");
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
	// LogMessage("getStatistics");
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
	// LogMessage("getLatency");
    return 100;
}

extern "C" DLLEXPORT
void commitTransaction(void *adapter)
{
	LogMessage("commitTransaction");
}

