#pragma once

#include "stdafx.h"

/*Adopted from
https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
*/




class IConcurrentQueue {
public:
	/*	virtual void push() = 0;
	virtual bool empty() = 0;
	virtual bool try_pop() = 0;
	virtual void wait_and_pop() = 0;*/
};


template<typename Data>
class ConcurrentQueue : public IConcurrentQueue
{
private:
	std::queue<Data> the_queue;
	mutable boost::mutex the_mutex;
	boost::condition_variable the_condition_variable;
public:
	void push(Data const& data)
	{
		boost::mutex::scoped_lock lock(the_mutex);
		the_queue.push(data);
		lock.unlock();
		the_condition_variable.notify_one();
	}

	bool empty() const
	{
		boost::mutex::scoped_lock lock(the_mutex);
		return the_queue.empty();
	}

	bool try_pop(Data& popped_value)
	{
		boost::mutex::scoped_lock lock(the_mutex);
		if (the_queue.empty())
		{
			return false;
		}

		popped_value = the_queue.front();
		the_queue.pop();
		return true;
	}

	void wait_and_pop(Data& popped_value)
	{
		boost::mutex::scoped_lock lock(the_mutex);
		while (the_queue.empty())
		{
			the_condition_variable.wait(lock);
		}

		popped_value = the_queue.front();
		the_queue.pop();
	}

};


typedef ConcurrentQueue<cv::Vec3f> TrackingQueue;		//used in tracking
typedef ConcurrentQueue<cv::Vec4f> CalibrationQueue;	//used in calibration