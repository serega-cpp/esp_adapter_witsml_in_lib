#define _CRT_SECURE_NO_WARNINGS
#include "SampleData.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>

#include "common/Utils.h"

SampleData::SampleData(size_t reserve_size) :
	m_sample_fields(reserve_size),
	m_time_idx(0),
	m_value_idx(0)
{
}

bool SampleData::FromCsvFile(const char *fname, char in_sep_csv, size_t fields_cnt, size_t time_idx, size_t value_idx)
{
	// Open sample file
	Utils::CsvFileReader sample_file(fname, in_sep_csv);
	if (!sample_file.HasOpened()) {
		std::cerr << "Error: failed to open sample file " << fname << "." << std::endl;
		return false;
	}

	// Skip file header
	sample_file.GetHeader();

	std::vector <std::string> file_line;
	size_t file_line_no = 0;

	// Load sample file content
	while (!sample_file.IsEof()) {

		size_t columns_cnt = sample_file.GetNextLine(file_line);
		file_line_no++;

		// If empty line encountered, stop reading
		if (columns_cnt == 0)
			break;

		// If wrong columns count encountered, stop application
		if (columns_cnt != fields_cnt) {
			std::cerr << "Error: Wrong fields count in file at " << file_line_no << " line." << std::endl;
			return false;
		}

		// Keep the first line as a source for other non-generated columns
		if (file_line_no == 1)
			m_const_fields.assign(file_line.begin(), file_line.end());

		// Extract Datetime and Value
		const char *date_str = file_line[time_idx].c_str();
		const char *value_str = file_line[value_idx].c_str();

		tm date_rec;
		if (!Utils::TextToDateTime(date_str, &date_rec)) {
			std::cerr << "Error: Failed to parse date: " << date_str << std::endl;
			return false;
		}

		time_t date = mktime(&date_rec);
		double value = atof(value_str);

		// Add Datetime and Value into storage
		m_sample_fields.AddElement(date, value);
	}

	// keep the generated fields indexes for row generator algorithm
	m_time_idx = time_idx;
	m_value_idx = value_idx;

	return true;
}

std::string SampleData::GenerateRow(time_t dst_time, char out_sep_csv)
{
	std::stringstream output;

	// Generate the value
	double value;
	if (!m_sample_fields.GetValue(dst_time, value))
		value = -1.0;

	// Prepare the date
	tm *t_rec = gmtime(&dst_time);
	std::string t_str = Utils::DateTimeToText(t_rec);

	// Build the row
	for (size_t i = 0; i < m_const_fields.size(); i++) {

		if (i == m_time_idx) output << t_str;
		else if (i == m_value_idx) output << value;
		else output << m_const_fields[i];

		if (i < m_const_fields.size() - 1)
			output << out_sep_csv;
		else
			output << '\n';
	}

	return output.str();
}

double SampleData::GenerateValue(time_t dst_time)
{
	double value;
	
    if (!m_sample_fields.GetValue(dst_time, value))
		value = -1.0;
    
    return value;
}
