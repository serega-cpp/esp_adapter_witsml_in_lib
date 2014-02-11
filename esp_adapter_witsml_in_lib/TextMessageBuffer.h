#ifndef __TEXTMESSAGEBUFFER_H__
#define __TEXTMESSAGEBUFFER_H__

#include <string>
#include "common/array_ptr.h"

class TextMessageBuffer
{
public:
	TextMessageBuffer(const char *separator);

	void AddString(const char *text, size_t size = 0);
	std::string GetNextMessage();

private:
	Utils::array_ptr<char>	m_buffer;
	size_t					m_size;
	size_t					m_eod_pos;

	std::string				m_msg_separator;
};

#endif // __TEXTMESSAGEBUFFER_H__
