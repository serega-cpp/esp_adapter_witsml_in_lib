#include <iostream>

#include "common/Utils.h"
#include "esp_adapter_witsml_in_lib/WitsmlProcessor.h"

// #define ORGVER

int main(int argc, char *argv[])
{
	const char *cDynamicXml = "../esp_adapter_witsml_in_lib/dynamic.xml";
	const char *cWitsmlXml = "../witsmler/example2.xml";

#ifdef ORGVER
	std::string witsml_str;
	if (!Utils::GetFileContent(cWitsmlXml, witsml_str)) {
		std::cerr << "Error: Failed open file: " << cWitsmlXml << std::endl;
		return 1;
	}

	std::vector<std::string> rows;
	if (!process_witsml(witsml_str, ';', rows)) {
		std::cerr << "Error: Witsml file processing failed" << std::endl;
		return 1;
	}

	if (!Utils::SetFileContent("C:\\Temp\\result_org.txt", rows)) {
		std::cerr << "Error: Failed create result file" << std::endl;
		return 1;
	}

#else
	GetKeyGen(1978);

	std::string witsml_rule_str;
	if (!Utils::GetFileContent(cDynamicXml, witsml_rule_str)) {
		std::cerr << "Error: Failed open file: " << cDynamicXml << std::endl;
		return 1;
	}

	WitsmlRule witsml_rule;
	if (!process_witsml_rule(witsml_rule_str, witsml_rule)) {
		std::cerr << "Error: Witsml rule processing failed" << std::endl;
		return 1;
	}

	std::string witsml_str;
	if (!Utils::GetFileContent(cWitsmlXml, witsml_str)) {
		std::cerr << "Error: Failed open file: " << cWitsmlXml << std::endl;
		return 1;
	}

	std::vector<std::string> rows;
	if (!process_witsml(witsml_str, witsml_rule, ';', rows)) {
		std::cerr << "Error: Witsml file processing failed" << std::endl;
		return 1;
	}

	rows.insert(rows.begin(), "id;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15\n");
	if (!Utils::SetFileContent("C:\\Temp\\result_excel.txt", rows)) {
		std::cerr << "Error: Failed create result file" << std::endl;
		return 1;
	}
#endif // ORGVER

	return 0;
}
