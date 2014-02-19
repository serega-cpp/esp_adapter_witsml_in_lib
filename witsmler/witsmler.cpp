// witsmler.cpp : Defines the entry point for the console application.
//

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstream>
#include <iostream>
#include <string>
#include "common/array_ptr.h"
#include "common/TcpClient.h"

size_t GetFileSize(const char *fname)
{
    struct stat stat_buf;
    int rc = stat(fname, &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

int main(int argc, char *argv[])
{
	if (argc != 4) {
		std::cerr << "Usage: witsmler xml_file esp_host esp_port" << std::endl;
		return 1;
	}

	const char *file_name = argv[1];
	const char *esp_host = argv[2];
	unsigned short esp_port = (unsigned short)atoi(argv[3]);

	size_t file_size = GetFileSize(file_name);
	if (file_size == 0 || file_size > 8 * 1024 * 1024) {
		std::cerr << "Error: xml_file has size " << file_size << std::endl;
		return 1;
	}

	const std::string eom_sign("&&");

	size_t buffer_size = file_size + eom_sign.length();
	Utils::array_ptr<char> buffer(new char [buffer_size]);

	std::ifstream file(file_name, std::ios::in | std::ios::binary);
    if(!file.is_open()) {
		std::cerr << "Error: failed to open file " << file_name << std::endl;
        return 1;
	}

	file.read(buffer.get(), file_size);
	file.close();

	memcpy(buffer.get() + file_size, eom_sign.c_str(), eom_sign.length());
	// std::cout << std::string(buffer.get(), buffer.get() + buffer_size) << std::endl;

	TcpClient esp_server;
	std::cerr << "Info: [" << esp_host << ":" << esp_port << "] Connecting...";
	while (!esp_server.Connect(esp_host, esp_port)) {
		std::cerr << '.';
		Sleep(333);
	}

	int cLoopCount = 1000;
	std::cerr << std::endl << "Info: Sending data to ESP (" << buffer_size << " x " << cLoopCount << " bytes)..." << std::endl;

	clock_t c1 = clock();

	size_t total_bytes = 0;
	for (int i = 0; i < cLoopCount; i++) {
		if (!esp_server.Send(buffer.get(), buffer_size)) {
			std::cerr << "Error: send to esp failed" << std::endl;
			break;
		}

		total_bytes += buffer_size;
	}

	clock_t c2 = clock();

	std::cerr << "Completed! (" << double(c2 - c1) / CLOCKS_PER_SEC << " sec, " 
								<< total_bytes << " bytes, " 
								<< total_bytes / (double(c2 - c1) / CLOCKS_PER_SEC) << " bytes/sec)" 
								<< std::endl;

	return 0;
}
