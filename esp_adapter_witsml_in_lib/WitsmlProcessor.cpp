#include "WitsmlProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "pugixml/pugixml.hpp"
#include "log_message.h"

const char cCsvDelimiter = ',';

///////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

#include <iostream>

void PrintTable(const Table &table)
{
    std::cout << table.GetName() << "[" << table.size() << "]" << std::endl; 

    for (size_t j = 0; j < table.size(); j++)
        std::cout << "  " << table.at(j).first << ":" << table.at(j).second << std::endl;
}

#endif // _DEBUG

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

    for (size_t i = 0; i < hashed_indexes.size(); i++)
        s[i % 4].append(fields[hashed_indexes[i]]);

    return hash(s[0], s[1], s[2], s[3]);
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

    if (type == WitsmlRule::Item::TextType) {
        size_t sep_pos = value.find(":");
        if (sep_pos != std::string::npos) {
            name.swap(value.substr(0, sep_pos));
            this->text_value.swap(value.substr(sep_pos + 1));
        }
        else this->text_value = value;
    }
    else if (type == WitsmlRule::Item::HashType) {
        process_csv_indexes(value, hashed_indexes);
    }
}

int WitsmlRule::FindMetaStaticColumn(const char *node_name, const char *value_name) const
{
    for (size_t idx = 0; idx < static_columns.size(); idx++) {
        if (Utils::strcmpi(node_name, static_columns[idx].name.c_str()) == 0 &&
            Utils::strcmpi(value_name, static_columns[idx].text_value.c_str()) == 0) 
            return idx;
    }

    return -1;
}

int WitsmlRule::FindMetaVariableColumn(const char *value_name) const
{
    for (size_t idx = 0; idx < variable_columns.size(); idx++) {
        if (Utils::strcmpi(value_name, variable_columns[idx].text_value.c_str()) == 0) 
            return idx;
    }

    return -1;
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

    // for (size_t i = 0; i < tables.size(); i++)
    //    PrintTable(tables[i]);

    for (std::vector<Table>::iterator table_it = tables.begin(); table_it != tables.end(); ++table_it) {
        const char *node_name = table_it->GetName();

        if (Utils::strcmpi(node_name, "ColumnsMeta") == 0) {
			for (Table::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {
                if (Utils::strcmpi(row_it->first.c_str(), "tableId") == 0) witsml_rule.meta_table_id.swap(row_it->second.c_str());
            }
        }
        else if (Utils::strcmpi(node_name, "StaticAttrs") == 0) {
			for (Table::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {

                if (Utils::strcmpi(row_it->first.c_str(), "NodeName") == 0) 
                    witsml_rule.static_columns.push_back(WitsmlRule::Item(WitsmlRule::Item::TextType, row_it->second));
                else if (Utils::strcmpi(row_it->first.c_str(), "Hash") == 0) 
                    witsml_rule.static_columns.push_back(WitsmlRule::Item(WitsmlRule::Item::HashType, row_it->second));
                else if (Utils::strcmpi(row_it->first.c_str(), "Key") == 0) 
                    process_csv_indexes(row_it->second, witsml_rule.static_key_indexes);
            }
        }
        else if (Utils::strcmpi(node_name, "VariableAttrs") == 0) {
			for (Table::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {

                if (Utils::strcmpi(row_it->first.c_str(), "NodeName") == 0) 
                    witsml_rule.variable_columns.push_back(WitsmlRule::Item(WitsmlRule::Item::TextType, row_it->second));
                else if (Utils::strcmpi(row_it->first.c_str(), "Hash") == 0) 
                    witsml_rule.variable_columns.push_back(WitsmlRule::Item(WitsmlRule::Item::HashType, row_it->second));
                else if (Utils::strcmpi(row_it->first.c_str(), "Key") == 0) 
                    process_csv_indexes(row_it->second, witsml_rule.variable_key_indexes);
                else if (Utils::strcmpi(row_it->first.c_str(), "collection") == 0) 
                    witsml_rule.variable_node_name.swap(row_it->second);
            }
        }
        else if (Utils::strcmpi(node_name, "ColumnsData") == 0) {
			for (Table::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {

                if (Utils::strcmpi(row_it->first.c_str(), "tableId") == 0) witsml_rule.data_table_id.swap(row_it->second.c_str());
            }
        }
        else if (Utils::strcmpi(node_name, "DataValues") == 0) {
			for (Table::iterator row_it = table_it->begin(); row_it != table_it->end(); ++row_it) {

                if (Utils::strcmpi(row_it->first.c_str(), "NodeName") == 0) 
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

        // for (size_t i = 0; i < witsml_tables.size(); i++)
        //    PrintTable(witsml_tables[i]);

        ///////////////////////////////////////////////////////////////////////
        // gather all values based on Witsml Rule Info

		for (std::vector<Table>::iterator witsml_table = witsml_tables.begin(); witsml_table != witsml_tables.end(); ++witsml_table) {
			const char *node_name = witsml_table->GetName();

            if (Utils::strcmpi(node_name, witsml_rule.variable_node_name.c_str()) == 0) {
                std::vector<std::string> vv(witsml_rule.variable_columns.size());

                for (Table::iterator row = witsml_table->begin(); row != witsml_table->end(); ++row) {
                    int idx = witsml_rule.FindMetaVariableColumn(row->first.c_str());
                    if (idx >= 0) vv[idx].swap(row->second);
                }

                for (size_t i = 0; i < vv.size(); i++) {
                    if (witsml_rule.variable_columns[i].type == WitsmlRule::Item::HashType)
                        vv[i].swap(process_hash_field(vv, witsml_rule.variable_columns[i].hashed_indexes));
                }

                variable_values.push_back(vv);
            }
            else if (Utils::strcmpi(node_name, witsml_rule.data_node_name.c_str()) == 0) {
				for (Table::iterator row = witsml_table->begin(); row != witsml_table->end(); ++row) {
					if (Utils::strcmpi(row->first.c_str(), witsml_rule.data_field_name.c_str()) == 0)
                        data_csv.push_back(row->second);
				}
            }
            else {
                for (Table::iterator row = witsml_table->begin(); row != witsml_table->end(); ++row) {
                    int idx = witsml_rule.FindMetaStaticColumn(node_name, row->first.c_str());
                    if (idx >= 0) static_values[idx].swap(row->second);
                }
            }
        }

        for (size_t i = 0; i < static_values.size(); i++) {
            if (witsml_rule.static_columns[i].type == WitsmlRule::Item::HashType)
                static_values[i].swap(process_hash_field(static_values, witsml_rule.static_columns[i].hashed_indexes));
        }

		if (variable_values.empty())
			continue;

        ///////////////////////////////////////////////////////////////////////
        // print Witsml as csv rows

        // (-1) skip first Time column because the time is in each row
		for (std::vector<std::vector<std::string> >::const_iterator column = variable_values.begin() + 1; column != variable_values.end(); ++column) {

            std::stringstream csv_row;
            csv_row << witsml_rule.meta_table_id << output_delimiter;

            for (size_t i = 0; i < static_values.size(); i++)
                csv_row << static_values[i] << output_delimiter;

            for (size_t i = 0; i < column->size(); i++)
                csv_row << column->at(i) << output_delimiter;

            csv_row << std::endl;
			rows.push_back(csv_row.str());
		}

        // (+1) add one column for TableID
        const size_t cOutputColumnCount = witsml_rule.static_columns.size() + witsml_rule.variable_columns.size() + 1;

		std::vector<std::string> data_values;
		for (std::vector<std::string>::const_iterator data_row = data_csv.begin(); data_row != data_csv.end(); ++data_row) {

			data_values.clear();
			Utils::SplitCsv(*data_row, cCsvDelimiter, data_values);
			if (data_values.size() < variable_values.size())
				continue;

            size_t filled_column_count = witsml_rule.static_key_indexes.size() + witsml_rule.variable_key_indexes.size() + 1 + 2;
			for (size_t column_idx = 1; column_idx < variable_values.size(); column_idx++) {
				
                std::stringstream csv_row;
                csv_row << witsml_rule.data_table_id << output_delimiter;

                for (size_t i = 0; i < witsml_rule.static_key_indexes.size(); i++)
                    csv_row << static_values[witsml_rule.static_key_indexes[i]] << output_delimiter;

                for (size_t i = 0; i < witsml_rule.variable_key_indexes.size(); i++)
                    csv_row << variable_values[column_idx].at(witsml_rule.variable_key_indexes[i]) << output_delimiter;

                csv_row << data_values[0] << output_delimiter;
                csv_row << data_values[column_idx] << output_delimiter;

                for (size_t i = filled_column_count; i < cOutputColumnCount; i++)
                    csv_row << output_delimiter;

                csv_row << std::endl;
				rows.push_back(csv_row.str());
			}
		}
	}

	return true;
}
