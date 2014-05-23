#include "BNvalues.h"

#include <algorithm>

bool cmp_predicate(const BNvalues::BNelement &lhs, const BNvalues::BNelement &rhs) { return lhs.bn_date < rhs.bn_date; }

BNvalues::BNvalues(size_t reserve_mem_size)
{
	if (reserve_mem_size > 0)
		m_bn_elements.reserve(reserve_mem_size);
}

void BNvalues::AddElement(time_t date, double value)
{
	BNelement elem;
	elem.bn_date = date;
	elem.bn_value = value;

	m_bn_elements.push_back(elem);
}

bool BNvalues::GetValue(time_t date, double &value)
{
	if (m_bn_elements.empty() ||
		date < m_bn_elements[0].bn_date ||
		date > m_bn_elements[m_bn_elements.size() - 1].bn_date) {
			return false;
	}

	BNelement elem;
	elem.bn_date = date;
	std::vector<BNelement>::const_iterator iter = std::lower_bound(m_bn_elements.begin(), m_bn_elements.end(), elem, cmp_predicate);

	if (iter != m_bn_elements.begin()) {
		double v0 = (iter - 1)->bn_value;
		double v1 = iter->bn_value;
		time_t d0 = (iter - 1)->bn_date;
		time_t d1 = iter->bn_date;

		// do piecewise-linear approximation
		value = double(date - d0) * (v1 - v0) / double(d1 - d0) + v0;
	}
	else value = iter->bn_value;

	return true;
}
