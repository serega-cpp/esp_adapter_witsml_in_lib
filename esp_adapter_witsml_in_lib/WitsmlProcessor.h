#ifndef __WITSMLPROCESSOR_H__
#define __WITSMLPROCESSOR_H__

#define _CRT_SECURE_NO_WARNINGS
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

struct LogCurveInfoRec {
	LogCurveInfoRec(const std::string &hash, unsigned int uint_columnIndex, const std::string &text_columnIndex, std::string startIndex, std::string endIndex): 
		hash(hash), 
		uint_columnIndex(uint_columnIndex), 
		text_columnIndex(text_columnIndex), 
		startIndex(startIndex), 
		endIndex(endIndex) {
	}

	std::string		hash;
	unsigned int	uint_columnIndex;
	std::string		text_columnIndex;
	std::string		startIndex;
	std::string		endIndex;
};

bool traverse_xml(pugi::xml_node_iterator root_it, std::pair<std::string, std::string> uid, std::vector<TableData> &tables);
std::string hash(const std::string &s1, const std::string &s2 = std::string());
std::string hash(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4 = std::string());

bool process_witsml(const std::string &witsml, char output_delimiter, std::vector<std::string> &rows);

#endif // __WITSMLPROCESSOR_H__