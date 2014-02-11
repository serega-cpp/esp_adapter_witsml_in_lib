#include "WitsmlProcessor.h"
#include <string.h>

void process_raw_child(const char *root_name, const std::pair<std::string, std::string> &uid, const std::pair<std::string, std::string> &child, std::vector<TableData> &tables)
{
	TableData active_table(root_name);

	active_table.push_back(uid);
	active_table.push_back(child);

	tables.push_back(active_table);
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

			std::string value;
			if (children_it->type() == pugi::node_element)
				value.assign(children_it->child_value());

			// std::cout << "child: name='" << children_it->name() << "', value='" << children_it->value() << "' " << std::endl;

			if (!traverse_xml(children_it, uid, tables)) {
				std::pair<std::string, std::string> child(children_it->name(), value);
				if (active_table.empty() || active_table.back().first != children_it->name()) active_table.push_back(child);
				else process_raw_child(active_table.GetName(), child, uid, tables);
			}
		}
	}

	if (active_table.size() == attr_count)
		return false;

	active_table.insert(active_table.begin(), uid);
	tables.push_back(active_table);

	return true;
}
