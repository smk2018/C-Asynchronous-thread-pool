#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(int min, int max) : m_maxThread(max), m_minThread(min), m_stop(false), m_curThread(min), m_idleThread(min)
{
	//�����������߳�
	m_manager = new thread(&ThreadPool::manager, this); 
	//���������߳�
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
	//�˴���{}��Ϊ���޶�locker�������õķ�Χ��{}�ڲ������õȵ�m_condition.notify_one()���֮��������
	{
		lock_guard<mutex> locker(m_queueMutex);
		m_tasks.emplace(task);
	}
	m_condition.notify_one(); //���ѵ����߳�
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
			//ÿ������2���߳�
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
		//lockerֻ���������{}���������̵߳���ǰ�Զ���������ʡ��Դ
		{
			unique_lock<mutex> locker(m_queueMutex);
			//�ж���������Ƿ�Ϊ�ջ����̳߳عرգ���while����if��Ϊ���ٴ�����ѭ�����ж��̳߳��Ƿ�Ϊ�գ�����������������ѭ��
			while (m_tasks.empty() && !m_stop)
			{
				m_condition.wait(locker);
				if (m_exitThread.load() > 0)
				{
					m_curThread--;
					m_exitThread--;
					cout << "------- �߳��˳���, ID: " << this_thread::get_id() << endl;
					return;
				}
			}
			if (!m_tasks.empty())
			{
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