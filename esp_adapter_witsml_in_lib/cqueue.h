#ifndef __CQUEUE_H__
#define __CQUEUE_H__

#include <queue>
#include <memory>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

template <class T>
class cqueue
{
public:
	cqueue(size_t queue_max_size);
	~cqueue();

	bool Push(T *item);
	bool Pop(std::auto_ptr<T> &item_ptr);

	bool WaitForMsg();
	bool WaitForFree();

	void Clear();

	size_t Size() { return m_queue.size(); }

private:
	std::queue<T *>			m_queue;
	size_t				m_queue_max_size;

	mutable boost::mutex		m_mutex;
	boost::condition_variable	m_queue_has_data_cv;
	boost::condition_variable	m_queue_has_free_space_cv;

	cqueue(const cqueue &);
	cqueue &operator = (const cqueue &);
};

#endif // __CQUEUE_H__
