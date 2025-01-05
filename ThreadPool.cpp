#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(int min, int max) : m_maxThread(max), m_minThread(min), m_stop(false), m_curThread(min), m_idleThread(min)
{
	//创建管理者线程
	m_manager = new thread(&ThreadPool::manager, this); 
	//创建工作线程
	for (int i = 0; i < min; i++)
	{
		//thread t(&ThreadPool::worker, this);
		//m_workers.emplace_back(t);
		m_workers.emplace_back(thread(&ThreadPool::worker, this));
	}
}

ThreadPool::~ThreadPool()
{

}

void ThreadPool::addTask(function<void(void)> task)
{
	//此处的{}是为了限定locker对象作用的范围在{}内部，不用等到m_condition.notify_one()语句之后再析构
	{
		lock_guard<mutex> locker(m_queueMutex);
		m_tasks.emplace(task);
	}
	m_condition.notify_one(); //唤醒单个线程
}

void ThreadPool::manager(void)
{
	while (!m_stop.load())
	{
		this_thread::sleep_for(chrono::seconds(3));
		int idle = m_idleThread.load();
		int cur = m_curThread.load();
		if (idle > cur/2 && cur > m_minThread)
		{
			//每次销毁2个线程
			m_exitThread.store(2);
			m_condition.notify_all();
		}
		else if (idle == 0 && cur < m_maxThread)
		{
			m_workers.emplace_back(thread(&ThreadPool::worker, this));
			m_curThread++;
			m_idleThread++;
		}
	}
}

void ThreadPool::worker(void)
{
	while (!m_stop.load())
	{
		function<void(void)> task = nullptr;
		//locker只作用于这个{}作用域，在线程调用前自动析构，节省资源
		{
			unique_lock<mutex> locker(m_queueMutex);
			//判断任务队列是否为空或者线程池关闭，用while不用if是为了再次跳出循环后判断线程池是否为空，继续阻塞，避免死循环
			while (m_tasks.empty() && !m_stop)
			{
				m_condition.wait(locker);
				if (m_exitThread.load() > 0)
				{
					m_curThread--;
					m_exitThread--;
					cout << "------- 线程退出了, ID: " << this_thread::get_id() << endl;
					return;
				}
			}
			if (!m_tasks.empty())
			{
				task = move(m_tasks.front()); //move是转移资源，不是复制资源，不浪费空间和资源
				m_tasks.pop(); //取出线程
			}
		}

		if (task) //调用线程
		{
			m_idleThread--;
			task();
			m_idleThread++;
		}
	}
}