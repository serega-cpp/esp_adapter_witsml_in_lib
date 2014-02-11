#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <vector>
#include <string>

namespace Utils
{

	///////////////////////////////////////////////////////////////////////////////
	//// Simple wrapper for disk file
	////
	class CTxtFile
	{
	public:
		CTxtFile();
		~CTxtFile();

		CTxtFile(const char *pszFileName, const char *pszMode); // ANSI version
		bool Open(const char *pszFileName, const char *pszMode); // ANSI version

#ifdef _WIN32
		CTxtFile(const wchar_t *pszFileName, const wchar_t *pszMode); // UNICODE version
		bool Open(const wchar_t *pszFileName, const wchar_t *pszMode); // UNICODE version
#endif // _WIN32
		
		FILE *GetHandle() { return m_f; }

		void Close();

	private:
		FILE *m_f;
	};

	///////////////////////////////////////////////////////////////////////////////
	//// CSV file wrapper
	////
	class CsvFileReader
	{
	public:
		CsvFileReader(const char *file_name, char field_sep);

		bool HasOpened();
		bool IsEof();

		char *GetHeader();

		size_t GetNextLine(char **toks, size_t toks_size);
		size_t GetNextLine(std::vector<std::string> &v);

	private:
		CTxtFile m_file;
		char m_line[1024];

		char	m_field_sep;
		size_t	m_fields_cnt;
	};

	///////////////////////////////////////////////////////////////////////////////
	template <typename T>
	void   SplitCsv(const std::basic_string<T> &s, T c, std::vector<std::basic_string<T> > &v);

	//////////////////////////////////////////////////////////////////////////
	size_t SplitCsv(char *s, const char sep, char **toks, size_t toks_size);

	///////////////////////////////////////////////////////////////////////////////
	bool TextToDateTime(const char *dt_str, struct tm *dt_rec);

	///////////////////////////////////////////////////////////////////////////////
	std::string DateTimeToText(const struct tm *dt_rec);

	///////////////////////////////////////////////////////////////////////////////
	const char *ExtractFileName(const char *full_path);

	char * itoa(int val, char *buf, int base);
	int strcmpi(const char *s1, const char *s2);

} // namespace Utils

#endif // __UTILS_H__
