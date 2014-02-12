#define _CRT_SECURE_NO_WARNINGS
#include "Utils.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <string.h>

#include <sstream>
#include <iomanip>

namespace Utils
{
	CTxtFile::CTxtFile() :
		m_f(NULL)
	{
	}

	CTxtFile::~CTxtFile()
	{
		Close();
	}

	CTxtFile::CTxtFile(const char *pszFileName, const char *pszMode) :
		m_f(NULL)
	{
		m_f = fopen(pszFileName, pszMode);
	}

	bool CTxtFile::Open(const char *pszFileName, const char *pszMode)
	{
		Close();

		m_f = fopen(pszFileName, pszMode);

		return m_f != NULL;
	}

#ifdef _WIN32

	CTxtFile::CTxtFile(const wchar_t *pszFileName, const wchar_t *pszMode) :
		m_f(NULL)
	{
		_wfopen_s(&m_f, pszFileName, pszMode);
	}

	bool CTxtFile::Open(const wchar_t *pszFileName, const wchar_t *pszMode)
	{
		Close();

		errno_t err = _wfopen_s(&m_f, pszFileName, pszMode);

		return err == 0 && m_f != NULL;
	}

#endif // _WIN32

	void CTxtFile::Close()
	{
		if (m_f != NULL)
		{
			fclose(m_f);
			m_f = NULL;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	CsvFileReader::CsvFileReader(const char *file_name, char field_sep) :
		m_file(file_name, "rt"),
		m_field_sep(field_sep)
	{
	}

	bool CsvFileReader::HasOpened()
	{
		return m_file.GetHandle() != NULL;
	}

	bool CsvFileReader::IsEof()
	{
		return feof(m_file.GetHandle()) != 0;
	}

	char *CsvFileReader::GetHeader()
	{
		if (!feof(m_file.GetHandle()))
		{
			if (fgets(m_line, sizeof(m_line), m_file.GetHandle()))
				return m_line;
		}

		return NULL;
	}

	size_t CsvFileReader::GetNextLine(char **toks, size_t toks_size)
	{
		size_t toks_red = 0;

		if (!feof(m_file.GetHandle()))
		{
			if (fgets(m_line, sizeof(m_line), m_file.GetHandle()))
				toks_red = SplitCsv(m_line, m_field_sep, toks, toks_size);
		}

		return toks_red;
	}

	size_t CsvFileReader::GetNextLine(std::vector<std::string> &v)
	{
		v.clear();

		if (!feof(m_file.GetHandle()))
		{
			if (fgets(m_line, sizeof(m_line), m_file.GetHandle()))
				SplitCsv(std::string(m_line), m_field_sep, v);
		}

		//// remove '\n' from last items if exists
		if (!v.empty())
		{
			size_t pos = v.back().rfind('\n');
			if (pos != std::string::npos)
				v.back().resize(pos);
		}

		return v.size();
	}

	//////////////////////////////////////////////////////////////////////////

	template <typename T>
	void SplitCsv(const std::basic_string<T> &s, T c, std::vector<std::basic_string<T> > &v)
	{
		size_t i_beg = 0;
		size_t i_end = s.find(c);

		while (i_end != s.npos)
		{
			v.push_back(s.substr(i_beg, i_end - i_beg));
			i_beg = i_end + 1; // skip separator
			i_end = s.find(c, i_beg);
		}

		if (size_t remain = s.length() - i_beg)
		{
			v.push_back(s.substr(i_beg, remain));
		}
	}

	template
		void SplitCsv(const std::basic_string<char> &s, char c, std::vector<std::basic_string<char> > &v);

	template
		void SplitCsv(const std::basic_string<wchar_t> &s, wchar_t c, std::vector<std::basic_string<wchar_t> > &v);

	//////////////////////////////////////////////////////////////////////////

	size_t SplitCsv(char *s, const char sep, char **toks, size_t toks_size)
	{
		size_t idx = 0;

		char *beg = s;
		char *tok = strchr(s, sep);
		while (tok != NULL && idx < toks_size)
		{
			toks[idx++] = beg;
			*tok = '\0';

			beg = tok + 1;
			tok = strchr(beg, sep);
		}

		//// if no separator found, return zero
		if (idx > 0) toks[idx++] = beg;

		return idx;
	}

	//////////////////////////////////////////////////////////////////////////

	bool TextToDateTime(const char *time_str, struct tm *dt_rec)
	{
		memset(dt_rec, 0, sizeof(*dt_rec));

		int cnt = sscanf(time_str, "%04d-%02d-%02d %02d:%02d:%02d",
			&dt_rec->tm_year, &dt_rec->tm_mon, &dt_rec->tm_mday,
			&dt_rec->tm_hour, &dt_rec->tm_min, &dt_rec->tm_sec);

		dt_rec->tm_year -= 1900;
		dt_rec->tm_mon -= 1;

		return (cnt == 6);
	}

	//////////////////////////////////////////////////////////////////////////

	std::string DateTimeToText(const struct tm *dt_rec)
	{
		std::stringstream ss;

		ss << std::setfill('0') <<
			dt_rec->tm_year + 1900 << '-' << std::setw(2) << dt_rec->tm_mon + 1 << '-' << std::setw(2) << dt_rec->tm_mday << ' ' <<
			std::setw(2) << dt_rec->tm_hour << ':' << std::setw(2) << dt_rec->tm_min << ':' << std::setw(2) << dt_rec->tm_sec;

		return ss.str();
	}

	const char *ExtractFileName(const char *full_path)
	{
		const char *executable_fname;
		if ((executable_fname = strrchr(full_path, '/')) || (executable_fname = strrchr(full_path, '\\')))
			return executable_fname + 1; // the next position after the slash

		return full_path; // seems the full_path is actually an executable file name
	}

	unsigned short Crc16(const unsigned char *block, size_t len)
	{
		// Name  : CRC-16 CCITT
		// Poly  : 0x1021    x^16 + x^12 + x^5 + 1
		unsigned short crc = 0xFFFF;
 
		while (len--) {
			crc ^= *block++ << 8;
 			for (int i = 0; i < 8; i++)
				crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
		}
 
		return crc;
	}

	char * itoa(int val, char *buf, int base)
	{
		char internal_buf[32] = { 0 };

		int i = 30;
		for (; val && i; --i, val /= base)
			internal_buf[i] = "0123456789abcdef"[val % base];
		strcpy(buf, internal_buf + i + 1);

		return buf;
	}

	int strcmpi (const char * dst, const char * src)
	{
		int f, l;

		do
		{
			if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
				f -= 'A' - 'a';
			if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
				l -= 'A' - 'a';
		}
		while ( f && (f == l) );

	    return(f - l);
	}

} // namespace Utils
