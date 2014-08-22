// witsmler.cpp : Defines the entry point for the console application.
//
#define _CRT_SECURE_NO_WARNINGS
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <boost/shared_ptr.hpp>
#include "pugixml/pugixml.hpp"

#include "common/array_ptr.h"
#include "common/TcpClient.h"
#include "common/Utils.h"

#include "SampleData.h"

size_t GetFileSize(const char *fname)
{
	struct stat stat_buf;
	int rc = stat(fname, &stat_buf);
	return rc == 0 ? stat_buf.st_size : 0;
}

void ReplaceOne(std::string& str, const std::string& from, const std::string& to) 
{
	size_t start_pos = str.find(from);
	if (start_pos != std::string::npos)
		str.replace(start_pos, from.length(), to);
}

void ReplaceAll(std::string& str, const std::string& from, const std::string& to) 
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); 
	}
}

std::string DatetimeToString(time_t t, size_t millisec = 0)
{
	//// sample row "2006-08-03 00:44:20.000"
	struct tm *tm_rec = localtime(&t);

	char tm_str[80];
	sprintf(tm_str, "%d-%02d-%02d %02d:%02d:%02d.%03d",
		tm_rec->tm_year + 1900, tm_rec->tm_mon + 1, tm_rec->tm_mday, 
		tm_rec->tm_hour, tm_rec->tm_min, tm_rec->tm_sec, millisec);

	return std::string(tm_str);
}

time_t StringToDatetime(const char *time_str)
{
	//// sample "2014-04-28_09:15:00" [YYYY-MM-DD_HH:MM:SS] len:19
	size_t time_len = strlen(time_str);
	if (time_len != 19) return 0;

	char dt_separator;
	struct tm rec = {0};
	int fields = sscanf(time_str, "%d-%d-%d%c%d:%d:%d", &rec.tm_year, &rec.tm_mon, &rec.tm_mday, &dt_separator, &rec.tm_hour, &rec.tm_min, &rec.tm_sec);
	if (fields != 7) return 0;

	rec.tm_year -= 1900;
	rec.tm_mon -= 1;
	return mktime(&rec);
}

void DumpGeneratedXml(const std::string &xml_body, time_t xml_time)
{
	char dbg_file_name[32];
	sprintf(dbg_file_name, "wtsm_%d.xml", (int)xml_time);
	Utils::SetFileContent(dbg_file_name, std::vector<std::string>(1, xml_body));
}

struct SettingsRec
{
	struct ColumnRec 
	{
		SampleData approx_data;
		size_t index;

		bool operator < (const ColumnRec &src) { return index < src.index; }
	};

	time_t begin_time;
	time_t end_time;
	std::string time_zone;
	double time_step_sec;
	time_t time_file_period_sec;

	std::vector< boost::shared_ptr<ColumnRec> > columns;
};

template <typename T>
bool shared_ptr_cmp_pred(const boost::shared_ptr<T> &a, const boost::shared_ptr<T> &b)
{
	return *a.get() < *b.get();
}

bool ReadSettings(const char *settings_fname, SettingsRec *settings_rec)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(settings_fname);

	if (!result) {
		std::cerr << "ERROR: ReadSettings: " << result.description() << " at offset: " << result.offset << std::endl;
		return false;
	}

	pugi::xml_node root_node = doc.child("settings");
	if (!root_node) {
		std::cerr << "ERROR: ReadSettings: parse error: <settings> not found" << std::endl;
		return false;
	}

	settings_rec->begin_time = StringToDatetime(root_node.child("beginTime").text().as_string());
	settings_rec->end_time = StringToDatetime(root_node.child("endTime").text().as_string());
	settings_rec->time_zone = root_node.child("timeZone").text().as_string();
	settings_rec->time_step_sec = root_node.child("timeStepSec").text().as_double();
	settings_rec->time_file_period_sec = root_node.child("timeFilePeriodSec").text().as_uint();

	if (settings_rec->begin_time == 0 || settings_rec->end_time == 0) {
		std::cerr << "ERROR: ReadSettings: Wrong time format: " << (settings_rec->begin_time == 0 ? "beginTime" : "endTime") << std::endl;
		return 1;
	}

	for (pugi::xml_node column_node = root_node.child("column"); column_node; column_node = column_node.next_sibling("column")) {
		std::string fname = column_node.child_value("file");

		boost::shared_ptr<SettingsRec::ColumnRec> column_rec(new SettingsRec::ColumnRec);
		column_rec->index = atoi(column_node.child_value("index"));
		column_rec->approx_data.FromCsvFile(fname.c_str(), '\t', 2, 0, 1);

		settings_rec->columns.push_back(column_rec);
	}
	
	std::sort(settings_rec->columns.begin(), settings_rec->columns.end(), shared_ptr_cmp_pred<SettingsRec::ColumnRec>);

	return true;
}

bool GenerateXml(std::string &xml, const SettingsRec &settings, time_t begin_time = 0, time_t end_time = 0)
{
	if (begin_time == 0) begin_time = settings.begin_time;
	if (end_time == 0) end_time = settings.end_time;
	if (begin_time > end_time) return false;

	std::string data_part;

	size_t sec_step = std::max(size_t(settings.time_step_sec), size_t(1));
	size_t ms_step = size_t(settings.time_step_sec * 1000);

	for (time_t t = begin_time; t <= end_time; t += sec_step) {

		for (size_t ms = 0; ms < 1000; ms += ms_step) {

			std::stringstream ss;
			ss << "<data>" << DatetimeToString(t, ms);
			for (size_t i = 0; i < settings.columns.size(); i++)
				ss << ',' << settings.columns[i].get()->approx_data.GenerateValue(t);
			ss << "</data>\n";

			data_part.append(ss.str());
		}
	}

	std::string start_time_str = DatetimeToString(begin_time);
	std::string end_time_str = DatetimeToString(end_time);
	
	ReplaceAll(xml, "@@STARTTIME@@", start_time_str);
	ReplaceAll(xml, "@@ENDTIME@@", end_time_str);
	ReplaceOne(xml, "@@DATA@@", data_part);
	ReplaceOne(xml, "@@TZ@@", settings.time_zone);

	return true;
}

int main(int argc, char *argv[])
{
	if (argc != 5) {
		std::cerr << "Usage:" << std::endl;
		std::cerr << "-> mode 1: send one WitsML file many times:" << std::endl;
		// sample command line arguments: localhost 12345 example2.xml 1000
		std::cerr << "   $witsmler esp_host esp_port xml_file times_count" << std::endl;
		std::cerr << std::endl;
		std::cerr << "-> mode 2: generate & send WitsML files based on template:" << std::endl;
		// sample command line arguments: localhost 12345 template2.xml settings.xml
		std::cerr << "   $witsmler esp_host esp_port xml_template settings_xml" << std::endl;

		return 1;
	}

	bool is_one_file_mode = atoi(argv[4]) != 0;

	const char *esp_host = argv[1];
	unsigned short esp_port = (unsigned short)atoi(argv[2]);
	const char *file_name = argv[3];

	//// Read XML file contents

	std::string file_body;
	if (!Utils::GetFileContent(file_name, file_body)) {
		std::cerr << "Error: failed to open file " << file_name << std::endl;
		return 1;
	}

	//// Connect to ESP

	TcpClient esp_client;
	std::cerr << "Info: [" << esp_host << ":" << esp_port << "] Connecting... ";
	while (!esp_client.Connect(esp_host, esp_port)) { std::cerr << '.'; Sleep(500); }

	//// Prepare data and send to ESP

	clock_t c1 = 0, c2 = 0;
	size_t total_bytes = 0;

	if (is_one_file_mode) {

		size_t loop_count = atoi(argv[4]);
		std::cerr << std::endl << "Info: Sending data to ESP (" << file_body.length() << " x " << loop_count << " bytes)..." << std::endl;

		c1 = clock();

		for (size_t i = 0; i < loop_count; i++) {
			if (!esp_client.Send(file_body.c_str(), file_body.length()) ||
				!esp_client.Send("&&", 2)) {
				std::cerr << "Error: send to esp failed" << std::endl;
				return 1;
			}

			total_bytes += file_body.length();
		}

		c2 = clock();
	}
	else {

		const char *settings_fname = argv[4];

		SettingsRec settings;
		if (!ReadSettings(settings_fname, &settings)) {
			std::cerr << "Template mode error: Failed to read settings from file (" << settings_fname << ")" << std::endl;
			return 1;
		}

		c1 = clock();

		for (time_t begin_time = settings.begin_time; begin_time < settings.end_time; begin_time += settings.time_file_period_sec) {
			time_t end_time = std::min(begin_time + settings.time_file_period_sec, settings.end_time);

			std::string generated_file_body(file_body);
			if (!GenerateXml(generated_file_body, settings, begin_time, end_time)) {
				std::cerr << "Error: GenerateXml failed at " << DatetimeToString(begin_time) << "." << std::endl;
				continue;
			}

			// Uncomment to save generated xml file into current directory
			// DumpGeneratedXml(generated_file_body, begin_time);

			if (!esp_client.Send(generated_file_body.c_str(), generated_file_body.length()) || !esp_client.Send("&&", 2)) {
				std::cerr << "Error: send to esp failed" << std::endl;
				return 1;
			}

			total_bytes += generated_file_body.length();
		}

		c2 = clock();
	}

	if (c1 == -1 || (c1 == 0 && c2 == 0))
		std::cerr << "Completed! (performance statistic is not available on platform)" << std::endl;
	else 
		std::cerr << "Completed! (" << double(c2 - c1) / CLOCKS_PER_SEC << " sec, " 
								<< total_bytes << " bytes, " 
								<< (double)total_bytes / (double(c2 - c1) / CLOCKS_PER_SEC) << " bytes/sec)" 
								<< std::endl;

	return 0;
}
