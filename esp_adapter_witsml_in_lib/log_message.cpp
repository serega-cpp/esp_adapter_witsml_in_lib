#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
	const char *cLogFilePrefix = "c:\\temp\\esp_adapter_";
#else
	const char *cLogFilePrefix = "/home/serega/esp_adapter_";
#endif

bool log_message(const char *class_name, const char *format_str, ...)
{
	return true;

	va_list arg;
	va_start(arg, format_str);

	std::stringstream fname_ss;
	fname_ss << cLogFilePrefix << class_name << ".log";

    time_t cur_time;
    time(&cur_time);

	std::stringstream format_ss;
	size_t day_time = cur_time % (3600 * 24);

	format_ss << std::setw(2) << std::setfill('0') << day_time / 3600 
			  << std::setw(2) << std::setfill('0') << (day_time / 60) % 60 
			  << std::setw(2) << std::setfill('0') << day_time % 60 << ' ' 
			  << format_str << "\n";

	FILE *f = fopen(fname_ss.str().c_str(), "at");
	if (!f) return false;

	vfprintf(f, format_ss.str().c_str(), arg);
	fclose(f);

	va_end(arg);

	return true;
}