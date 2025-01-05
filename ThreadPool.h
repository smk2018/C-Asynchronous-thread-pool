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
#include <map>
#include <condition_variable>
#include <future>
using namespace std;

class ThreadPool
{
public:
	ThreadPool(int min = 4, int max = thread::hardware_concurrency());
	~ThreadPool();

	//添加任务 -> 任务队列
	void addTask(function<void()> f);
	template<typename F, typename... Args>
	auto addTask(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type>
	{
		// 1.packaged_task
		// 2. 得到future
		// 3. 任务函数添加到任务队列
		using return_type = typename result_of<F(Args...)>::type;
		auto task = make_shared<packaged_task<return_type()>>(bind(forward<F>(f), forward<Args>(args)...));
		future<return_type> res = task->get_future();
		{
			unique_lock<mutex> lock(m_queueMutex);
			m_tasks.emplace([task]() { (*task)(); });
		}
		m_condition.notify_one();
		return res;
	}

private:
	void manager(); 
	void worker(); 
private:
	thread* m_manager;
	map<thread::id, thread> m_workers; //存储工作线程
	vector<thread::id> m_ids; //存储已经退出了任务函数的线程ID
	int m_minThread;  //最小的线程数量
	int m_maxThread;  //最大的线程数量
	atomic<int> m_curThread;  //当前的线程数量
	atomic<int> m_idleThread; //空闲的线程数量
	atomic<int> m_exitThread; //退出的线程数量
	atomic<bool> m_stop; //线程池开关
	queue<function<void()>> m_tasks; //任务队列
	mutex m_queueMutex; //任务队列的互斥锁
	mutex m_idsMutex; //线程ID的互斥锁
	condition_variable m_condition; //条件变量
};