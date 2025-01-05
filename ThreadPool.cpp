#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(int min, int max) : m_maxThread(max), m_minThread(min), m_stop(false), m_exitThread(0)
{
	m_idleThread = m_curThread = min;
	cout << "线程数量: " << m_curThread << endl;
	//创建管理者线程
	m_manager = new thread(&ThreadPool::manager, this); 
	//创建工作线程
	for (int i = 0; i < m_curThread; ++i)
	{
		//thread t(&ThreadPool::worker, this);
		//m_workers.emplace_back(t);
		thread t(&ThreadPool::worker, this);
		m_workers.insert(make_pair(t.get_id(), move(t)));
	}
}

ThreadPool::~ThreadPool()
{
	m_stop = true; //关闭线程池
	m_condition.notify_all(); //唤醒所有线程
	for (auto& it : m_workers)
	{
		thread& t = it.second;
		if (t.joinable()) 
		{
			cout << "******* 线程 " << t.get_id() << "将要退出了 ..." << endl;
			t.join(); //阻塞当前的线程，等待子线程执行完毕
		}
	}
	if (m_manager->joinable())
	{
		m_manager->join();
	}
	delete m_manager;
}

void ThreadPool::addTask(function<void()> f)
{
	//此处的{}是为了限定locker对象作用的范围在{}内部，不用等到m_condition.notify_one()语句之后再析构
	{
		lock_guard<mutex> locker(m_queueMutex);
		m_tasks.emplace(f);
	}
	m_condition.notify_one(); //唤醒单个线程
}

void ThreadPool::manager()
{
	while (!m_stop.load())
	{
		this_thread::sleep_for(chrono::seconds(2));
		int idle = m_idleThread.load();
		int cur = m_curThread.load();
		if (idle > cur/2 && cur > m_minThread)
		{
			//每次销毁2个线程
			m_exitThread.store(2);
			m_condition.notify_all();
			unique_lock<mutex> lck(m_idsMutex);
			for (const auto& id : m_ids)
			{
				auto it = m_workers.find(id);
				if (it != m_workers.end())
				{
					cout << "######## 线程 " << (*it).first << "被销毁了 ..." << endl;
					(*it).second.join();
					m_workers.erase(it);
				}
			}
			m_ids.clear();
		}
		else if (idle == 0 && cur < m_maxThread)
		{
			thread t(&ThreadPool::worker, this);
			cout << "+++++++++++++++ 添加了一个线程, id: " << t.get_id() << endl;
			m_workers.insert(make_pair(t.get_id(), move(t)));
			m_curThread++;
			m_idleThread++;
		}
	}
}

void ThreadPool::worker()
{
	while (!m_stop.load())
	{
		function<void()> task = nullptr;
		//locker只作用于这个{}作用域，在线程调用前自动析构，节省资源
		{
			unique_lock<mutex> locker(m_queueMutex);
			//判断任务队列是否为空或者线程池关闭，用while不用if是为了再次跳出循环后判断线程池是否为空，继续阻塞，避免死循环
			while (m_tasks.empty() && !m_stop)
			{
				m_condition.wait(locker);
				if (m_exitThread.load() > 0)
				{
					cout << "------------ 线程任务结束, ID: " << this_thread::get_id() << endl;
					m_curThread--;
					m_idleThread--;
					m_exitThread--;
					unique_lock<mutex> lck(m_idsMutex);
					m_ids.emplace_back(this_thread::get_id());
					return;
				}
			}
			if (!m_tasks.empty())
			{
				cout << "取出了一个任务 ..." << endl;
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

//void calc(int x, int y)
//{
//	int z = x + y;
//	cout << "z = " << z << endl;
//	this_thread::sleep_for(chrono::seconds(2));
//}

int calc(int x, int y)
{
	int res = x + y;
	//cout << "res = " << res << endl;
	this_thread::sleep_for(chrono::seconds(2));
	return res;
}

int main()
{
	ThreadPool pool(4);
	vector<future<int>> results;
	for (int i = 0; i < 10; ++i)
	{
		//auto obj = bind(calc, i, i * 2);
		//pool.addTask(obj);
		results.emplace_back(pool.addTask(calc, i, i * 2));
	}
	//getchar(); //等待线程执行完毕
	for (auto& res : results)
	{
		cout << "线程函数返回值：" << res.get() << endl;
	}
	return 0;
}