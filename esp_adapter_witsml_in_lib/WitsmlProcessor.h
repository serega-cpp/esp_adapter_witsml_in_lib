#ifndef __WITSMLPROCESSOR_H__
#define __WITSMLPROCESSOR_H__

#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <string>
#include <utility>
#include <sstream>

#include <boost/atomic.hpp>
#include "pugixml/pugixml.hpp"
#include "common/Utils.h"

struct WitsmlRule
{
    struct Item
    {
        enum Type { 
            TextType, 
            TimeType,
            TzType,
            HashCalcType
        };

        std::string         name;
        Type                type;

        std::string         text_value;
        std::vector<size_t> indexes;

        Item(Type type, const std::string &value);
    };

    std::string         meta_table_id;

    std::vector<Item>   static_columns;
    std::vector<size_t> static_key_indexes;

    std::string         variable_node_name;
    std::vector<Item>   variable_columns;
    std::vector<size_t> variable_key_indexes;

    std::string         data_table_id;
    std::string         data_node_name;
    std::string         data_field_name;

    int  FindMetaStaticColumn(const char *node_name, const char *value_name) const;
    int  FindMetaVariableColumn(const char *value_name) const;
};

class Table: public std::vector<std::pair<std::string, std::string> >
{
public:
	Table(const char *table_name = 0) {
		if (table_name) m_table_name.assign(table_name);
	}

	const char *GetName() const {
		return m_table_name.c_str();
	}

private:
	std::string m_table_name;
};

class KeyGen
{
public:
    KeyGen(unsigned int salt);
    std::string GetNext();

private:
    unsigned int m_salt;
    boost::atomic<unsigned int> m_inc;
};

KeyGen &GetKeyGen(unsigned int salt = 0);

bool traverse_xml(const pugi::xml_node_iterator &node, std::vector<Table> &tables);
bool process_witsml_rule(const std::string &rule_text, WitsmlRule &witsml_rule);
bool process_witsml(const std::string &witsml, const WitsmlRule &witsml_rule, char output_delimiter, std::vector<std::string> &rows);

#endif // __WITSMLPROCESSOR_H__
