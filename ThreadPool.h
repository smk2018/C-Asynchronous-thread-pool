#pragma once
/*
* ���ɣ�
* 1.�������߳� -> ���̣߳�1��
*       - ���ƹ����̵߳�������+/-
* 2.���ɹ����߳� -> ���̣߳�N��
*       - �����������ȡ���񣬲�����
*       - �������Ϊ�գ�������(��������������)
*       - �߳�ͬ��(������)
*       - ��ǰ���������е��߳�����
*       - Max/Min �߳�����
* 3.������� -> stl -> queue
*       - ������
*       - ��������
* 4.�̳߳ؿ��� -> bool
*/
#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
using namespace std;

class ThreadPool
{
public:
	ThreadPool(int min = 4, int max = thread::hardware_concurrency());
	~ThreadPool();

	//������� -> �������
	void addTask(function<void(void)> task);
private:
	void manager(void);
	void worker(void);
private:
	thread* m_manager;
	vector<thread> m_workers;
	atomic<int> m_minThread;
	atomic<int> m_maxThread;
	atomic<int> m_curThread;
	atomic<int> m_idleThread;
	atomic<int> m_exitThread;
	atomic<bool> m_stop;
	queue<function<void(void)>> m_tasks;
	mutex m_queueMutex;
	condition_variable m_condition;
};