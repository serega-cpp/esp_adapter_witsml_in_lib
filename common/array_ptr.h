#ifndef __ARAY_PTR_H__
#define __ARAY_PTR_H__

namespace Utils
{

template <class T>
class array_ptr
{
public:
	explicit array_ptr(T *ptr = 0):
		m_ptr(ptr)
	{
	}

	~array_ptr()
	{
		delete [] m_ptr;
		m_ptr = 0;
	}

	T * get() const
	{
		return m_ptr;
	}

	void reset(T *ptr = 0)
	{
		if(ptr != m_ptr)
			delete [] m_ptr;
		m_ptr = ptr;
	}

	T * release()
	{
		T *ptr = m_ptr;
		m_ptr = 0;
		return ptr;
	}

	bool empty() const
	{
		return m_ptr == 0;
	}

	operator T * () const
	{
		return m_ptr;
	}

private:
	T *m_ptr;
};

} // namespace Utils

#endif // __ARAY_PTR_H__
