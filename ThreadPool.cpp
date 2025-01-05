#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(int min, int max) : m_maxThread(max), m_minThread(min), m_stop(false), m_exitThread(0)
{
	m_idleThread = m_curThread = min;
	cout << "�߳�����: " << m_curThread << endl;
	//�����������߳�
	m_manager = new thread(&ThreadPool::manager, this); 
	//���������߳�
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
	m_stop = true; //�ر��̳߳�
	m_condition.notify_all(); //���������߳�
	for (auto& it : m_workers)
	{
		thread& t = it.second;
		if (t.joinable()) 
		{
			cout << "******* �߳� " << t.get_id() << "��Ҫ�˳��� ..." << endl;
			t.join(); //������ǰ���̣߳��ȴ����߳�ִ�����
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
	//�˴���{}��Ϊ���޶�locker�������õķ�Χ��{}�ڲ������õȵ�m_condition.notify_one()���֮��������
	{
		lock_guard<mutex> locker(m_queueMutex);
		m_tasks.emplace(f);
	}
	m_condition.notify_one(); //���ѵ����߳�
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
			//ÿ������2���߳�
			m_exitThread.store(2);
			m_condition.notify_all();
			unique_lock<mutex> lck(m_idsMutex);
			for (const auto& id : m_ids)
			{
				auto it = m_workers.find(id);
				if (it != m_workers.end())
				{
					cout << "######## �߳� " << (*it).first << "�������� ..." << endl;
					(*it).second.join();
					m_workers.erase(it);
				}
			}
			m_ids.clear();
		}
		else if (idle == 0 && cur < m_maxThread)
		{
			thread t(&ThreadPool::worker, this);
			cout << "+++++++++++++++ �����һ���߳�, id: " << t.get_id() << endl;
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
		//lockerֻ���������{}���������̵߳���ǰ�Զ���������ʡ��Դ
		{
			unique_lock<mutex> locker(m_queueMutex);
			//�ж���������Ƿ�Ϊ�ջ����̳߳عرգ���while����if��Ϊ���ٴ�����ѭ�����ж��̳߳��Ƿ�Ϊ�գ�����������������ѭ��
			while (m_tasks.empty() && !m_stop)
			{
				m_condition.wait(locker);
				if (m_exitThread.load() > 0)
				{
					cout << "------------ �߳��������, ID: " << this_thread::get_id() << endl;
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
				cout << "ȡ����һ������ ..." << endl;
				task = move(m_tasks.front()); //move��ת����Դ�����Ǹ�����Դ�����˷ѿռ����Դ
				m_tasks.pop(); //ȡ���߳�
			}
		}

		if (task) //�����߳�
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
	//getchar(); //�ȴ��߳�ִ�����
	for (auto& res : results)
	{
		cout << "�̺߳�������ֵ��" << res.get() << endl;
	}
	return 0;
}