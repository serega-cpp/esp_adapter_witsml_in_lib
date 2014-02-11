#define _CRT_SECURE_NO_WARNINGS
#include "TextMessageBuffer.h"
#include <assert.h>
#include <string.h>

TextMessageBuffer::TextMessageBuffer(const char *separator):
	m_size(0),
	m_eod_pos(0),
	m_msg_separator(separator)
{
	// separator do not included into messages, just skipped
}

void TextMessageBuffer::AddString(const char *text, size_t len)
{
	if (len == 0) len = strlen(text);
	assert(text[len] == '\0');

	if (m_eod_pos + len >= m_size) {
		size_t new_size = (m_size + len) * 2;
		Utils::array_ptr<char> new_buffer(new char [new_size]);

		if (!m_buffer.empty())
			strcpy(new_buffer.get(), m_buffer.get());

		m_buffer.reset(new_buffer.release());
		m_size = new_size;
	}

	strcpy(m_buffer.get() + m_eod_pos, text);
	m_eod_pos += len;
}

std::string TextMessageBuffer::GetNextMessage()
{
	if (m_buffer.empty()) return std::string();

	const char *eom = strstr(m_buffer.get(), m_msg_separator.c_str());
	if (!eom) return std::string();

	std::string msg(static_cast<const char *>(m_buffer.get()), eom);

	const char *next_msg = eom + m_msg_separator.length();
	size_t next_msg_len = strlen(next_msg);

	memmove(m_buffer.get(), next_msg, next_msg_len + 1);
	m_eod_pos = next_msg_len;

	return msg;
}
