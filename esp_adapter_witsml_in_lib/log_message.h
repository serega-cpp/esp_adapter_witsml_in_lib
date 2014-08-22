#ifndef __LOG_MESSAGE_H__
#define __LOG_MESSAGE_H__

#include <stddef.h>

class Log
{
public:
	enum Severity { Error, Info, Debug };

	Log(): _is_enabled(false) { _directory[0] = '\0'; }

	bool log_message(const char *class_name, Severity severity, const char *format_str, ...);
	void apply_settings();
	bool is_enabled();

private:
	bool _is_enabled;
	char _directory[512];

	static bool has_trailing_slash(const char *path, size_t len);
};

Log &log();

#endif // __LOG_MESSAGE_H__
