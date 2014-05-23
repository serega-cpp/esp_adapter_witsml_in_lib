#ifndef __SAMPLEDATA_H__
#define __SAMPLEDATA_H__

#include <time.h>
#include <string>
#include <vector>

#include "BNvalues.h"

class SampleData
{
public:
	SampleData(size_t reserve_size = 0);

	bool FromCsvFile(const char *fname, char in_sep_csv, size_t fields_cnt, size_t time_idx, size_t value_idx);
	std::string GenerateRow(time_t dst_time, char out_sep_csv);
	double GenerateValue(time_t dst_time);

	time_t GetFirstTime() { return m_sample_fields.GetFirstDate(); }
	time_t GetLastTime() { return m_sample_fields.GetLastDate(); }

private:
	BNvalues					m_sample_fields;	// sample data for data generation (datetime & value)
	std::vector <std::string>	m_const_fields;		// const non-generated fields (currently it's 1-st row of file)

	size_t						m_time_idx;
	size_t						m_value_idx;
};

#endif // __SAMPLEDATA_H__
