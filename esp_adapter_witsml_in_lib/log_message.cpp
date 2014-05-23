#define _CRT_SECURE_NO_WARNINGS
#include "log_message.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include <string>
#include <sstream>
#include <iomanip>

#include "pugixml/pugixml.hpp"
#include "common/Utils.h"

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
	const char cPathSlash = '\\';
#else
	const char cPathSlash = '/';
#endif

static const char *cLogFilePrefix = "esp_adapter_";

///////////////////////////////////////////////////////////////////////////////

bool Log::log_message(const char *class_name, Severity severity, const char *format_str, ...)
{
    if (!_is_enabled) return true;

	va_list arg;
	va_start(arg, format_str);

	std::stringstream fname_ss;
	fname_ss << _directory << cLogFilePrefix << class_name << ".log";

	FILE *f = fopen(fname_ss.str().c_str(), "at");
	if (!f) return false;

    time_t cur_time;
    time(&cur_time);

	std::stringstream format_ss;
	size_t day_time = cur_time % (3600 * 24);

    const char *svt_msg = "";
    switch (severity) {
    case Error:    
        svt_msg = "<Error>"; 
        break;
    case Info:     
        svt_msg = "<Info>";  
        break;
    case Debug:    
        svt_msg = "<Debug>"; 
        break;
    }

	format_ss << std::setw(2) << std::setfill('0') << day_time / 3600 << ':'
			  << std::setw(2) << std::setfill('0') << (day_time / 60) % 60  << ':'
			  << std::setw(2) << std::setfill('0') << day_time % 60 << ' ' 
              << svt_msg << ' ' << format_str << "\n";

	vfprintf(f, format_ss.str().c_str(), arg);
	fclose(f);

	va_end(arg);

	return true;
}

void Log::apply_settings()
{
#ifdef _DEBUG
    // #include <windows.h>
    // ::MessageBox(NULL, L"log_apply_settings", L"DEBUG", 0);
#endif // _DEBUG

    std::stringstream settings_path;
    const char *esp_home_dir = getenv("ESP_HOME");
    if (size_t home_dir_len = strlen(esp_home_dir)) {

        settings_path << esp_home_dir;
        if (!has_trailing_slash(esp_home_dir, home_dir_len)) settings_path << cPathSlash;
        settings_path << "lib" << cPathSlash << "adapters" << cPathSlash << "witsml_in.cnxml";

	    pugi::xml_document xml_doc;
        pugi::xml_parse_result result = xml_doc.load_file(settings_path.str().c_str());
	    if (!result) return;

	    pugi::xml_node root_node = xml_doc.child("Adapter");
	    if (!root_node) return;

        pugi::xml_node sections_node = root_node.child("Special");
        if (!sections_node) return;

        for (pugi::xml_node param_node = sections_node.child("Internal"); param_node; param_node = param_node.next_sibling("Internal")) {
            if (Utils::strcmpi(param_node.attribute("id").as_string(), "x_InternalLogEnable") == 0) {
                _is_enabled = param_node.attribute("default").as_bool();
            }
            else if (Utils::strcmpi(param_node.attribute("id").as_string(), "x_InternalLogPath") == 0) {
                const char *log_path = param_node.attribute("default").as_string();

                size_t path_len = strlen(log_path);
                if (path_len > 0 && path_len < sizeof(_directory) - 1) {
                    strcpy(_directory, log_path);

                    if (!has_trailing_slash(_directory, path_len))
                        _directory[path_len] = cPathSlash;
                }
            }
        }
    }
}

bool Log::is_enabled()
{
    return _is_enabled;
}

bool Log::has_trailing_slash(const char *path, size_t len)
{
    return len > 0 ? (path[len - 1] == '\\' || path[len - 1] == '/') : false;
}

///////////////////////////////////////////////////////////////////////////////

Log theLog;

Log &log()
{
    return theLog;
}
