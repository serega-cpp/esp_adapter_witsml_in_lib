#include "WitsmlProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

#include "pugixml/pugixml.hpp"
#include "log_message.h"

const char cCsvDelimiter = ',';

///////////////////////////////////////////////////////////////////////////////

void PrintTable(const Table &table)
{
    std::cout << table.GetName() << "[" << table.size() << "]" << std::endl; 

    for (size_t j = 0; j < table.size(); j++)
        std::cout << "  " << table.at(j).first << ":" << table.at(j).second << std::endl;
}

///////////////////////////////////////////////////////////////////////////////

std::string hash(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4)
{
	std::stringstream ss;

	unsigned short int crc1 = Utils::Crc16(reinterpret_cast<const unsigned char *>(s1.c_str()), s1.length());
    if (!s2.empty()) {

        unsigned short int crc2 = Utils::Crc16(reinterpret_cast<const unsigned char *>(s2.c_str()), s2.length());
        if (!s3.empty()) {
	    
            unsigned short int crc3 = Utils::Crc16(reinterpret_cast<const unsigned char *>(s3.c_str()), s3.length());
            if (!s4.empty()) {
        	
                unsigned short int crc4 = Utils::Crc16(reinterpret_cast<const unsigned char *>(s4.c_str()), s4.length());
                ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << crc1 << crc2 << crc3 << crc4;
            }
            else ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << crc1 << crc2 << crc3;
        }
        else ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << crc1 << crc2;
    }
    else ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << crc1;

	return ss.str();
}

std::string process_hash_field(const std::vector<std::string> &fields, const std::vector<size_t> &hashed_indexes)
{
    std::string s[4];

    for (size_t i = 0; i < hashed_indexes.size(); i++) {
        if (hashed_indexes[i] >= fields.size())
            return std::string();
        s[i % 4].append(fields[hashed_indexes[i]]);
    }

    return hash(s[0], s[1], s[2], s[3]);
}

size_t rfind_one_of(const std::string &str, char ch1, char ch2)
{
    size_t pos = str.length();

    while (pos-- > 0) {
        if (str[pos] == '+' || str[pos] == '-')
            break;
    }

    return pos;
}

std::string process_time_field(const std::string &field)
{
    size_t cMinDateTimeLength = 16;

    size_t pos = rfind_one_of(field, '+', '-');
    if (pos == size_t(-1) || pos < cMinDateTimeLength)
        return field;

    return field.substr(0, pos);
}

std::string process_timezone_field(const std::string &field)
{
    size_t cMinDateTimeLength = 16;

    size_t pos = rfind_one_of(field, '+', '-');
    if (pos == size_t(-1) || pos < cMinDateTimeLength)
        return std::string();

    return field.substr(pos);
}

void process_csv_indexes(const std::string &positions_csv, std::vector<size_t> &indexes)
{
    std::vector<std::string> hashed_fields;
    Utils::SplitCsv(positions_csv, cCsvDelimiter, hashed_fields);
    for (size_t idx = 0; idx < hashed_fields.size(); idx++)
        indexes.push_back(atoi(hashed_fields[idx].c_str()) - 1);
}

///////////////////////////////////////////////////////////////////////////////

WitsmlRule::Item::Item(WitsmlRule::Item::Type type, const std::string &value): type(type) {

    if (type == WitsmlRule::Item::TextType || type == WitsmlRule::Item::TimeType || type == WitsmlRule::Item::TzType) {
        size_t sep_pos = value.find(":");
        if (sep_pos != std::string::npos) {
            name.assign(value.substr(0, sep_pos));
            text_value.assign(value.substr(sep_pos + 1));
        }
        else text_value = value;
    }
    else if (type == WitsmlRule::Item::HashCalcType) {
        process_csv_indexes(value, indexes);
    }
}

///////////////////////////////////////////////////////////////////////////////

KeyGen::KeyGen(unsigned int salt)
    : m_salt(salt)
    , m_inc(0) 
{
}

std::string KeyGen::GetNext() 
{
    unsigned int pk = m_inc.fetch_add(1) + 1;

    std::stringstream keyss;
    keyss << pk << m_salt;
    return keyss.str();
}

KeyGen &GetKeyGen(unsigned int salt)
{
    // do not protect object creation in multithread environment, 
    // because we sure, the creation will be performed in single
    // thread before multithreaded using...
    static KeyGen obj(salt);

    return obj;
}

///////////////////////////////////////////////////////////////////////////////

bool traverse_xml(const pugi::xml_node_iterator &node, std::vector<Table> &tables)
{
	Table active_table(node->name());

	for (pugi::xml_attribute_iterator attr_it = node->attributes().begin(); attr_it != node->attributes().end(); ++attr_it) {

		active_table.push_back(std::pair<std::string, std::string>(attr_it->name(), attr_it->value()));
	}

	for (pugi::xml_node_iterator children_it = node->children().begin(); children_it != node->children().end(); ++children_it) {

		if (children_it->type() == pugi::node_element) {

			if (!traverse_xml(children_it, tables)) {
				std::pair<std::string, std::string> child(children_it->name(), children_it->child_value());
				active_table.push_back(child);
			}
		}
	}

    if (active_table.empty())
        return false;

	tables.push_back(active_table);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool process_witsml_rule(const std::string &rule_text, WitsmlRule &witsml_rule)
{
    if (rule_text.empty()) return false;
	pugi::xml_document	xml_doc;

	// parse xml
	pugi::xml_parse_result result = xml_doc.load_buffer(rule_text.c_str(), rule_text.length());
	if (!result) return false;
 
	pugi::xml_node_iterator rule = xml_doc.children().begin();

	std::vector<Table> tables;
	if (!traverse_xml(rule, tables) && tables.empty())
		return false;

    // Uncomment to print xml-based tables to stdout
    // for (size_t i = 0; i < tables.size(); i++)
    //    PrintTable(tables[i]);

    for (std::vector<Table>::iterator table_it = tables.begin(); table_it != tables.end(); ++table_it) {
        const char *node_name = table_it->GetName();

        if (Utils::strcmpi(node_name, "ColumnsMeta") == 0) {
            for (Table::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {
                if (Utils::strcmpi(row_it->first.c_str(), "tableId") == 0) witsml_rule.meta_table_id.swap(row_it->second);
            }
        }
        else if (Utils::strcmpi(node_name, "StaticAttrs") == 0 || Utils::strcmpi(node_name, "VariableAttrs") == 0) {
            bool is_static_node = node_name[11] == '\0'; // Dirty hack!
            std::vector<WitsmlRule::Item> &columns = is_static_node ? witsml_rule.static_columns : witsml_rule.variable_columns;
            std::vector<size_t> &keys = is_static_node ? witsml_rule.static_key_indexes : witsml_rule.variable_key_indexes;

			for (Table::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {

                if (Utils::strcmpi(row_it->first.c_str(), "PlainText") == 0) 
                    columns.push_back(WitsmlRule::Item(WitsmlRule::Item::TextType, row_it->second));
                else if (Utils::strcmpi(row_it->first.c_str(), "Time") == 0) 
                    columns.push_back(WitsmlRule::Item(WitsmlRule::Item::TimeType, row_it->second));
                else if (Utils::strcmpi(row_it->first.c_str(), "Hash") == 0) 
                    columns.push_back(WitsmlRule::Item(WitsmlRule::Item::HashCalcType, row_it->second));
                else if (Utils::strcmpi(row_it->first.c_str(), "TimeZone") == 0) 
                    columns.push_back(WitsmlRule::Item(WitsmlRule::Item::TzType, row_it->second));
                else if (Utils::strcmpi(row_it->first.c_str(), "Key") == 0) 
                    process_csv_indexes(row_it->second, keys);

                else if (Utils::strcmpi(row_it->first.c_str(), "collection") == 0) {
                    if (!is_static_node) witsml_rule.variable_node_name.swap(row_it->second);
                }
            }

            for (size_t i = 0; i < keys.size(); i++) {
                if (keys[i] >= columns.size())
                    log().log_message("WitsmlProcessor", Log::Error,
                        "WitsmlRules parser: Key index for %s attrs out of range: %d",
                        is_static_node ? "static" : "variable", keys[i]);
            }

            for (size_t i = 0; i < columns.size(); i++) {
                for (int j = 0; j < columns[i].indexes.size(); j++) {
                    if (columns[i].indexes[j] >= columns.size())
                        log().log_message("WitsmlProcessor", Log::Error,
                            "WitsmlRules parser: Hash index for %s attrs out of range %d",
                            is_static_node ? "static" : "variable", columns[i].indexes[j]);
                }
            }
        }
        else if (Utils::strcmpi(node_name, "ColumnsData") == 0) {
			for (Table::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {

                if (Utils::strcmpi(row_it->first.c_str(), "tableId") == 0) 
                    witsml_rule.data_table_id.swap(row_it->second);
                else if (Utils::strcmpi(row_it->first.c_str(), "CsvText") == 0) 
                    witsml_rule.data_field_name.swap(row_it->second);
                else if (Utils::strcmpi(row_it->first.c_str(), "collection") == 0) 
                    witsml_rule.data_node_name.swap(row_it->second);
            }
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool process_witsml(const std::string &witsml, const WitsmlRule &witsml_rule, char output_delimiter, std::vector<std::string> &rows)
{
    if (witsml.empty()) return false;
	pugi::xml_document	xml_doc;

	// parse xml
	pugi::xml_parse_result result = xml_doc.load_buffer(witsml.c_str(), witsml.length());
	if (!result) return false;
 
    // go though all root nodes (usually only one)
	pugi::xml_node_iterator nodes = xml_doc.children().begin();
	for (pugi::xml_node_iterator node = nodes->children().begin(); node != nodes->children().end(); ++node) {
		
        std::vector<std::string> static_values(witsml_rule.static_columns.size());
        std::vector<std::vector<std::string> > variable_values;
        std::vector<std::string> data_csv;

        std::vector<Table> witsml_tables;
		if (!traverse_xml(node, witsml_tables) && witsml_tables.empty())
			continue;

        // Uncomment to print xml-based tables to stdout
        // for (size_t i = 0; i < witsml_tables.size(); i++)
        //    PrintTable(witsml_tables[i]);

        ///////////////////////////////////////////////////////////////////////
        // process Witsml based on Rule Info

		for (std::vector<Table>::iterator witsml_table = witsml_tables.begin(); witsml_table != witsml_tables.end(); ++witsml_table) {
			const char *node_name = witsml_table->GetName();

            // process variable part of attributes
            if (Utils::strcmpi(node_name, witsml_rule.variable_node_name.c_str()) == 0) {
                variable_values.resize(variable_values.size() + 1);
                std::vector<std::string> &vv = variable_values.back();
                vv.resize(witsml_rule.variable_columns.size());

                // get all attributes except HASH which is calculated on second pass
                for (Table::iterator row = witsml_table->begin(); row != witsml_table->end(); ++row) {

                    for (size_t idx = 0; idx < witsml_rule.variable_columns.size(); idx++) {
                        if (Utils::strcmpi(row->first.c_str(), witsml_rule.variable_columns[idx].text_value.c_str()) == 0) {
                            if (witsml_rule.variable_columns[idx].type == WitsmlRule::Item::TextType)
                                vv[idx].assign(row->second);
                            else if (witsml_rule.variable_columns[idx].type == WitsmlRule::Item::TimeType)
                                vv[idx].assign(process_time_field(row->second));
                            else if (witsml_rule.variable_columns[idx].type == WitsmlRule::Item::TzType)
                                vv[idx].assign(process_timezone_field(row->second));
                        }
                    }
                }

                // process calculated HASH attributes 
                for (size_t i = 0; i < vv.size(); i++) {
                    if (witsml_rule.variable_columns[i].type == WitsmlRule::Item::HashCalcType)
                        vv[i].assign(process_hash_field(vv, witsml_rule.variable_columns[i].indexes));
                }
            }
            // process data values
            else if (Utils::strcmpi(node_name, witsml_rule.data_node_name.c_str()) == 0) {
				for (Table::iterator row = witsml_table->begin(); row != witsml_table->end(); ++row) {
					if (Utils::strcmpi(row->first.c_str(), witsml_rule.data_field_name.c_str()) == 0)
                        data_csv.push_back(row->second);
				}
            }
            // process static part of attributes
            else {
                // get all attributes except HASH which is calculated on second pass
                for (Table::iterator row = witsml_table->begin(); row != witsml_table->end(); ++row) {
                    for (size_t idx = 0; idx < witsml_rule.static_columns.size(); idx++) {
                        if (Utils::strcmpi(node_name, witsml_rule.static_columns[idx].name.c_str()) == 0 &&
                            Utils::strcmpi(row->first.c_str(), witsml_rule.static_columns[idx].text_value.c_str()) == 0) {

                            if (witsml_rule.static_columns[idx].type == WitsmlRule::Item::TextType)
                                static_values[idx].assign(row->second);
                            else if (witsml_rule.static_columns[idx].type == WitsmlRule::Item::TimeType)
                                static_values[idx].assign(process_time_field(row->second));
                            else if (witsml_rule.static_columns[idx].type == WitsmlRule::Item::TzType)
                                static_values[idx].assign(process_timezone_field(row->second));
                        }
                    }
                }
            }
        }

        // process static HASH calculated attributes
        for (size_t i = 0; i < static_values.size(); i++) {
            if (witsml_rule.static_columns[i].type == WitsmlRule::Item::HashCalcType)
                static_values[i].assign(process_hash_field(static_values, witsml_rule.static_columns[i].indexes));
        }

        // if no columns found, skip this Witsml
		if (variable_values.empty())
			continue;

        ///////////////////////////////////////////////////////////////////////
        // print Witsml as csv rows
        // note: row have to ends with value, not separator (e.g. v1;v2;...;vN)

        // create rows for ColumnMeta information (skip first column - Time)
		for (std::vector<std::vector<std::string> >::const_iterator column = variable_values.begin() + 1; column != variable_values.end(); ++column) {

            std::stringstream csv_row;

            // put TableId (Meta)
            csv_row << witsml_rule.meta_table_id;

            // put Primary Key, required by ESP Input Window
            csv_row << output_delimiter << GetKeyGen().GetNext();

            // put Static attributes
            for (size_t i = 0; i < static_values.size(); i++)
                csv_row << output_delimiter << static_values[i];

            // put Variable attributes
            for (size_t i = 0; i < column->size(); i++)
                csv_row << output_delimiter << column->at(i);

            csv_row << std::endl;
			rows.push_back(csv_row.str());
		}

        const size_t cOutputColumnCount = witsml_rule.static_columns.size() + witsml_rule.variable_columns.size() + 2; // +2 columns: TableID, PK

        // create rows for DataValues information
		std::vector<std::string> data_values;
		for (std::vector<std::string>::const_iterator data_row = data_csv.begin(); data_row != data_csv.end(); ++data_row) {

			data_values.clear();
			Utils::SplitCsv(*data_row, cCsvDelimiter, data_values);
			if (data_values.size() < variable_values.size())
				continue;

            size_t filled_column_count = witsml_rule.static_key_indexes.size() + witsml_rule.variable_key_indexes.size() + 2 + 2;
			for (size_t column_idx = 1; column_idx < variable_values.size(); column_idx++) {
				
                std::stringstream csv_row;

                // put TableId (Values)
                csv_row << witsml_rule.data_table_id;

                // put Primary Key, required by ESP Input Window
                csv_row << output_delimiter << GetKeyGen().GetNext();

                // put Static part of Key
                for (size_t i = 0; i < witsml_rule.static_key_indexes.size(); i++) {
                    if (witsml_rule.static_key_indexes[i] < static_values.size())
                        csv_row << output_delimiter << static_values[witsml_rule.static_key_indexes[i]];
                    else
                        csv_row << output_delimiter << "";
                }

                // put Variable part of Key
                for (size_t i = 0; i < witsml_rule.variable_key_indexes.size(); i++) {
                    if (witsml_rule.variable_key_indexes[i] < variable_values.size())
                        csv_row << output_delimiter << variable_values[column_idx].at(witsml_rule.variable_key_indexes[i]);
                    else
                        csv_row << output_delimiter << "";
                }

                // put value (time and value)
                csv_row << output_delimiter << process_time_field(data_values[0]);
                csv_row << output_delimiter << data_values[column_idx];

                // put empty separators to reach cOutputColumnCount
                for (size_t i = filled_column_count; i < cOutputColumnCount; i++)
                    csv_row << output_delimiter;

                csv_row << std::endl;
				rows.push_back(csv_row.str());
			}
		}
	}

	return true;
}
