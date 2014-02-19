#include "WitsmlProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iomanip>
#include <sstream>

#include "pugixml/pugixml.hpp"
#include "log_message.h"

std::string hash(const std::string &s1, const std::string &s2)
{
	unsigned short int crc1 = Utils::Crc16(reinterpret_cast<const unsigned char *>(s1.c_str()), s1.length());
	unsigned short int crc2 = Utils::Crc16(reinterpret_cast<const unsigned char *>(s2.c_str()), s2.length());

	std::stringstream ss;
	if (s2.empty()) ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << crc1;
	else ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << crc1 << crc2;

	return ss.str();
}

std::string hash(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4)
{
	return hash(s1, s2) + hash(s3, s4);
}

bool traverse_xml(pugi::xml_node_iterator root_it, std::pair<std::string, std::string> uid, std::vector<TableData> &tables)
{
	TableData active_table(root_it->name());
	bool is_root_table = uid.first.empty();

	if (is_root_table) {
		uid.first.assign("uid");
		uid.first.append(root_it->name());
	}

	for (pugi::xml_attribute_iterator attr_it = root_it->attributes().begin(); attr_it != root_it->attributes().end(); ++attr_it) {

		// looking for 'main ID' value, which will be used for all tables
		if (is_root_table && Utils::strcmpi(attr_it->name(), uid.first.c_str()) == 0) {
			uid.second.assign(attr_it->value());
			continue;
		}

		active_table.push_back(std::pair<std::string, std::string>(attr_it->name(), attr_it->value()));
	}

	size_t attr_count = active_table.size();

	for (pugi::xml_node_iterator children_it = root_it->children().begin(); children_it != root_it->children().end(); ++children_it) {

		if (children_it->type() == pugi::node_element) {

			if (!traverse_xml(children_it, uid, tables)) {
				std::pair<std::string, std::string> child(children_it->name(), children_it->child_value());

				if (active_table.empty() || active_table.back().first != children_it->name()) {
					active_table.push_back(child);
				}
				else {
					TableData second_active_table(active_table.GetName());

					active_table.push_back(child);
					active_table.push_back(uid);

					tables.push_back(second_active_table);
				}
			}
		}
	}

	if (active_table.size() == attr_count)
		return false;

	active_table.insert(active_table.begin(), uid);
	tables.push_back(active_table);

	return true;
}

bool process_witsml(const std::string &witsml, char output_delimiter, std::vector<std::string> &rows)
{
	int cColumnInfoTableId = 1001;
	int cDataLogTableId = 1002;

	pugi::xml_document	xml_doc;

	// parse xml
	pugi::xml_parse_result result = xml_doc.load_buffer(witsml.c_str(), witsml.length());
	if (!result) return false;

	pugi::xml_node_iterator root = xml_doc.children().begin();
	for (pugi::xml_node_iterator root_it = root->children().begin(); root_it != root->children().end(); ++root_it) {
		
		std::vector<TableData> tables;
		if (!traverse_xml(root_it, std::pair<std::string, std::string>(), tables))
			continue;

		// get UID (it always at first position)
		assert(!tables.empty());
		std::string uid(tables[0].at(0).second);

		std::string log_NameWell;
		std::string log_NameWellbore;
		std::string logHeader_UomNamingSystem;
		std::string commonData_NameSource;
		std::vector<LogCurveInfoRec> logCurveInfo;
		std::vector<std::string> logData;

		for (std::vector<TableData>::iterator table_it = tables.begin(); table_it != tables.end(); ++table_it) {
			std::string group(table_it->GetName());
			log_message("WitsmlProcessor", group.c_str());

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

				std::string	hash_postfix = hash(mnemonic);

				LogCurveInfoRec rec(hash_postfix, columnIndex, std::string(mnemAlias + ": " + curveDescription), startIndex, endIndex);
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

		std::string hash_prefix = hash(log_NameWell, log_NameWellbore, logHeader_UomNamingSystem, commonData_NameSource);

		for (std::vector<LogCurveInfoRec>::const_iterator info_it = logCurveInfo.begin() + 1; info_it != logCurveInfo.end(); ++info_it) {
			std::stringstream csv_row;

			csv_row << cColumnInfoTableId << output_delimiter			// table_id [ColumnInfo]
					<< output_delimiter									// timestamp
					<< output_delimiter									// data value
					<< info_it->uint_columnIndex << output_delimiter	// column index [shared]
					<< hash_prefix << output_delimiter					// hash
					<< info_it->text_columnIndex << output_delimiter	// column text
					<< info_it->startIndex << output_delimiter			// start index
					<< info_it->endIndex << output_delimiter			// end index
					<< log_NameWell << output_delimiter					// name well
					<< log_NameWellbore << output_delimiter				// name wellbore
					<< logHeader_UomNamingSystem << output_delimiter	// Uom name system
					<< commonData_NameSource << std::endl;				// name source

			rows.push_back(csv_row.str());
		}

		std::vector<std::string> fieldsData;
		for (std::vector<std::string>::const_iterator data_it = logData.begin(); data_it != logData.end(); ++data_it) {

			fieldsData.clear();
			const char logData_delimiter = ',';
			Utils::SplitCsv(*data_it, logData_delimiter, fieldsData);
			if (fieldsData.size() < logCurveInfo.size())
				continue;

			for (std::vector<LogCurveInfoRec>::const_iterator info_it = logCurveInfo.begin() + 1; info_it != logCurveInfo.end(); ++info_it) {
				std::stringstream csv_row;

				csv_row << cDataLogTableId << output_delimiter				// table_id [DataLog]
						<< fieldsData[0] << output_delimiter				// timestamp
						<< fieldsData[info_it->uint_columnIndex] << output_delimiter	// data value
						<< info_it->uint_columnIndex << output_delimiter	// column index [shared]
						<< output_delimiter									// hash
						<< output_delimiter									// column text
						<< output_delimiter									// start index
						<< output_delimiter									// end index
						<< output_delimiter									// name well
						<< output_delimiter									// name wellbore
						<< output_delimiter									// Uom name system
						<< std::endl;										// name source

				rows.push_back(csv_row.str());
			}
		}
	}

	return true;
}
