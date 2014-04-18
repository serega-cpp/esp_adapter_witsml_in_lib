// witsmler.cpp : Defines the entry point for the console application.
//
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "common/array_ptr.h"
#include "common/TcpClient.h"
#include "common/Utils.h"

size_t GetFileSize(const char *fname)
{
    struct stat stat_buf;
    int rc = stat(fname, &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

void ReplaceAll(std::string& str, const std::string& from, const std::string& to) 
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); 
    }
}

std::string DatetimeToString(time_t t, size_t millisec = 0)
{
    const int tz_hour = 2; // timezone offset

    struct tm *tm_rec = localtime(&t);

    char tm_str[80];
    //// sample row "2006-08-03T00:44:20.000+02:00"
    sprintf(tm_str, "%d-%02d-%02dT%02d:%02d:%02d.%03d+%02d:00",
        tm_rec->tm_year + 1900, tm_rec->tm_mon + 1, tm_rec->tm_mday, 
        tm_rec->tm_hour, tm_rec->tm_min, tm_rec->tm_sec, millisec, tz_hour);

    return std::string(tm_str);
}

bool GenerateXml(std::string &xml, 
                 size_t columns_cnt, 
                 time_t start_time,
                 time_t end_time, 
                 size_t rows_per_sec)
{
    std::string data_part;
    for (time_t t = start_time; t <= end_time; t++) {

        for (size_t ms_idx = 0; ms_idx < rows_per_sec; ms_idx++) {
            size_t ms = 1000 / rows_per_sec * ms_idx;

            std::stringstream ss;
            ss << "<data>" << DatetimeToString(t, ms);
            for (size_t i = 0; i < columns_cnt; i++)
                ss << ',' << (double(rand() % 512) * 0.01);
            ss << "</data>\n";

            data_part.append(ss.str());
        }
    }

    std::string start_time_str = DatetimeToString(start_time);
    std::string end_time_str = DatetimeToString(end_time);

    ReplaceAll(xml, "@@STARTTIME@@", start_time_str);
    ReplaceAll(xml, "@@ENDTIME@@", end_time_str);
    ReplaceAll(xml, "@@DATA@@", data_part);

    return true;
}

int main(int argc, char *argv[])
{
    bool is_one_file_mode = (argc == 5);
    bool is_template_mode = (argc == 8);

	if (!is_one_file_mode && !is_template_mode) {
		std::cerr << "Usage:" << std::endl;
        // sample: localhost 12345 example2.xml 10
		std::cerr << "\\>witsmler esp_host esp_port xml_file loop_count" << std::endl;
		std::cerr << "or" << std::endl;
        // sample: localhost 1978 template2.xml 15 1398214800 1398214900 2
		std::cerr << "\\>witsmler esp_host esp_port xml_template_file columns_cnt start_time end_time per_sec" << std::endl;

		return 1;
	}

	const char *    esp_host = argv[1];
	unsigned short  esp_port = (unsigned short)atoi(argv[2]);
	const char *    file_name = argv[3];

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
			    break;
		    }
		    total_bytes += file_body.length();
	    }

	    c2 = clock();
    }
    else if (is_template_mode) {

        size_t columns_cnt = atoi(argv[4]);
        time_t start_time = atoi(argv[5]);
        time_t end_time = atoi(argv[6]);
        size_t per_sec = atoi(argv[7]);

        // calculate file period 30s or 60s
        time_t file_period = per_sec > 0 ? 30 : 60;

	    std::cerr << std::endl << "Info: Sending data to ESP (" << (end_time - start_time + 1) * per_sec << " rows " <<
            " by " << file_period << " seconds in " << (end_time - start_time) / file_period + 1 << " files)..." << std::endl;

	    c1 = clock();

        for (time_t t = start_time; t < end_time; t += file_period) {
            std::string generated_file_body(file_body);
            GenerateXml(generated_file_body, columns_cnt, t, t + file_period, per_sec);

		    if (!esp_client.Send(generated_file_body.c_str(), generated_file_body.length()) ||
                !esp_client.Send("&&", 2)) {
			    std::cerr << "Error: send to esp failed" << std::endl;
			    break;
		    }

		    total_bytes += generated_file_body.length();
        }

	    c2 = clock();
    }

    if (c1 == -1 || (c1 == 0 && c2 == 0))
		std::cerr << "Completed! (performance statistic not available on platform)" << std::endl;
	else 
		std::cerr << "Completed! (" << double(c2 - c1) / CLOCKS_PER_SEC << " sec, " 
								<< total_bytes << " bytes, " 
								<< (double)total_bytes / (double(c2 - c1) / CLOCKS_PER_SEC) << " bytes/sec)" 
								<< std::endl;

	return 0;
}
