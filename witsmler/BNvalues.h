#ifndef __BNVALUES_H__
#define __BNVALUES_H__

#include <time.h>
#include <vector>

class BNvalues
{
public:
	struct BNelement {
		time_t bn_date;
		double bn_value;
	};

	BNvalues(size_t reserve_mem_size = 0);

	void AddElement(time_t date, double value);
	bool GetValue(time_t date, double &value);

	time_t GetFirstDate() { return m_bn_elements.empty() ? 0 : m_bn_elements[0].bn_date; }
	time_t GetLastDate() { return m_bn_elements.empty() ? 0 : m_bn_elements[m_bn_elements.size() - 1].bn_date; }


private:
	std::vector <BNelement> m_bn_elements;
};

#endif // __BNVALUES_H__
