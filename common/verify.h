#ifndef __VERIFY_H__
#define __VERIFY_H__

#include <assert.h>

inline
int VERIFY(int val)
{
	assert(val);
	return val;
}

#endif // __VERIFY_H__
