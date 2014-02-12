#ifndef __WITSMLPROCESSOR_H__
#define __WITSMLPROCESSOR_H__

#include <vector>
#include <string>
#include <utility>
#include <sstream>

#include "pugixml/pugixml.hpp"
#include "common/Utils.h"

class TableData: public std::vector<std::pair<std::string, std::string> >
{
public:
	TableData(const char *table_name = 0) {
		if (table_name) m_table_name.assign(table_name);
	}

	const char *GetName() {
		return m_table_name.c_str();
	}

private:
	std::string m_table_name;
};

void process_raw_child(const char *root_name, const std::pair<std::string, std::string> &uid, const std::pair<std::string, std::string> &child, std::vector<TableData> &tables);
bool traverse_xml(pugi::xml_node_iterator root_it, std::pair<std::string, std::string> uid, std::vector<TableData> &tables);
std::string hash(const std::string &s1, const std::string &s2 = std::string());
std::string hash(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4 = std::string());

struct LogCurveInfoRec {
	LogCurveInfoRec(const std::string &hash2, unsigned int uint_columnIndex, const std::string &text_columnIndex, std::string startIndex, std::string endIndex): 
		hash2(hash2), 
		uint_columnIndex(uint_columnIndex), 
		text_columnIndex(text_columnIndex), 
		startIndex(startIndex), 
		endIndex(endIndex) {
	}

	std::string		hash2;
	unsigned int	uint_columnIndex;
	std::string		text_columnIndex;
	std::string		startIndex;
	std::string		endIndex;
};


#endif // __WITSMLPROCESSOR_H__