#include "cqueue.h"
#include <boost/thread/locks.hpp>

template <class T>
cqueue<T>::cqueue(size_t queue_max_size):
	m_queue_max_size(queue_max_size)
{
}

template <class T>
cqueue<T>::~cqueue()
{
	Clear();
}

template <class T>
bool cqueue<T>::Push(T *item)
{
	boost::mutex::scoped_lock lock(m_mutex);

	if(m_queue.size() == m_queue_max_size)
		return false;

	m_queue.push(item);
	lock.unlock();

	m_queue_has_data_cv.notify_one();

	return true;
}

template <class T>
bool cqueue<T>::Pop(std::auto_ptr<T> &item_ptr)
{
	boost::mutex::scoped_lock lock(m_mutex);
	
	if(m_queue.empty())
		return false;

	item_ptr.reset(m_queue.front());
	
	m_queue.pop();
	lock.unlock();

	if(m_queue.size() < m_queue_max_size * 9 / 10)
		m_queue_has_free_space_cv.notify_all();
	
	return true;
}

template <class T>
bool cqueue<T>::WaitForMsg()
{
	if(!m_queue.empty())
		return true;

	//// wait on dedicated cv (can be interrupted at any time
	//// by System or Close(), so return result must be checked)
	boost::mutex::scoped_lock lock(m_mutex);
	m_queue_has_data_cv.wait(lock);

	return !m_queue.empty();
}

template <class T>
bool cqueue<T>::WaitForFree()
{
	if(m_queue.size() < m_queue_max_size)
		return true;

	//// wait on dedicated cv (can be interrupted at any time
	//// by System or Close(), so return result must be checked)
	boost::mutex::scoped_lock lock(m_mutex);
	m_queue_has_free_space_cv.wait(lock);

	return m_queue.size() < m_queue_max_size;
}

template <class T>
void cqueue<T>::Clear()
{
	//// wake up the all waiters
	m_queue_has_free_space_cv.notify_all();
	m_queue_has_data_cv.notify_all();

	//// clear content
	boost::lock_guard<boost::mutex> lock(m_mutex);
	while (!m_queue.empty()) {
		delete m_queue.front();
		m_queue.pop();
	}
}

#include <string>
template class cqueue <std::string>;
