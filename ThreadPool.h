#pragma once
/*
* 构成：
* 1.管理者线程 -> 子线程，1个
*       - 控制工作线程的数量：+/-
* 2.若干工作线程 -> 子线程，N个
*       - 从任务队列中取任务，并处理
*       - 任务队列为空，被阻塞(被条件变量阻塞)
*       - 线程同步(互斥锁)
*       - 当前数量，空闲的线程数量
*       - Max/Min 线程数量
* 3.任务队列 -> stl -> queue
*       - 互斥锁
*       - 条件变量
* 4.线程池开关 -> bool
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

	//添加任务 -> 任务队列
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