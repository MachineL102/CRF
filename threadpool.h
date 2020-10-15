/*
	优化之前的参考线程池实现，在该文件中添加了更详细的注释，论述了有哪些地方需要优化，
	只有对比旧实现，才知道为什么需要新实现，是这个文件存在的意义。
	reference：https://github.com/lzpong/threadpool/blob/master/threadpool.h
	需要优化的部分：读写锁，线程安全的队列
	更详细的注释包括：为什么使用读写锁、为什么使用更细粒度线程安全的队列、
	如何提高并发性能的小技巧、对github评论（该库没有并发）的回答
*/
#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <future>
#include "threadsafe_queue.h"

namespace std
{
#define  THREADPOOL_MAX_NUM 16
//#define  THREADPOOL_AUTO_GROW
	class threadpool
	{
		using Task = function<void()>;	//定义类型
		vector<thread> _pool;     //线程池
		threadsafe_queue<Task> _tasks;            //任务队列
		// 由于多个非主线程读任务队列，所以需要读写锁的实现，保证同一时刻可以有多个线程同时读取_tasks。
		// （如果主线程在写入_tasks，所有线程都不能读取_tasks，更多详情请学习读写锁）
		mutex _lock;                   //同步，整个任务队列的读写权限（如果）
		condition_variable _task_cv;   //条件阻塞
		atomic<bool> _run{ true };     //线程池是否执行（共享数据）
		atomic<int>  _idlThrNum{ 0 };  //空闲线程数量（共享数据）

	public:
		inline threadpool(unsigned short size = 4) { addThread(size); }
		inline ~threadpool()
		{
			_run = false;
			_task_cv.notify_all(); // 唤醒所有线程执行
			for (thread& thread : _pool) {
				//thread.detach(); // 让线程“自生自灭”
				if (thread.joinable())
					thread.join(); // 等待任务结束， 前提：线程一定会执行完
			}
		}

		// 将任务加入task_但并不一定立即执行
		template<class F, class... Args>
		auto commit(F&& f, Args&&... args) ->future<decltype(f(args...))>
		{
			if (!_run)    // stoped ??
				throw runtime_error("commit on ThreadPool is stopped.");

			using RetType = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
			auto task = make_shared<packaged_task<RetType()>>(
				bind(forward<F>(f), forward<Args>(args)...)	// forward：防止临时对象的右值引用被当作左值来处理
				); // 把函数入口及参数,打包(绑定)
			future<RetType> future = task->get_future(); // 这里也没有执行任务，注意和future调用get方法不要混淆
			{    // 添加任务到队列
				lock_guard<mutex> lock{ _lock };//对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
				_tasks.push([task]() { // push(Task{...}) 放到队列后面
					(*task)();
					});
			}
#ifdef THREADPOOL_AUTO_GROW
			if (_idlThrNum < 1 && _pool.size() < THREADPOOL_MAX_NUM)
				addThread(1);
#endif // !THREADPOOL_AUTO_GROW
			_task_cv.notify_one(); // 唤醒一个线程执行队列头部的任务（不是本次commit的任务），（不存在伪唤醒）

			return future;
		}

		int idlCount() { return _idlThrNum; }
		int thrCount() { return _pool.size(); }
#ifndef THREADPOOL_AUTO_GROW
	private:
#endif // !THREADPOOL_AUTO_GROW
		void addThread(unsigned short size)
		{
			for (; _pool.size() < THREADPOOL_MAX_NUM && size > 0; --size)
			{   //增加线程数量,但不超过 预定义数量 THREADPOOL_MAX_NUM
				_pool.emplace_back([this] { // 添加工作线程（非主线程）函数，直接开始运行，直到_run == false
					while (_run)
					{
						// 获取一个待执行的 task
						Task task; // 在锁住互斥元之前进行内存分配，减少数据竞争
						{
							// unique_lock 相比 lock_guard 的好处是：可以随时 unlock() 和 lock()
							unique_lock<mutex> lock{ _lock };	// 每一个非主线程访问任务队列需要读权限
							// 如果条件不满足，则调用unlock()并阻塞该线程（主线程）
							_task_cv.wait(lock, [this] {
								return !_run || !_tasks.empty();
								}); // wait 直到有 task
							if (!_run && _tasks.empty())
								return;
							task = *_tasks.wait_and_pop(); // 线程阻塞直到有任务弹出
						}	// unlock()，任务队列访问完毕
						_idlThrNum--;
						// 执行任务，此处是在非主线程执行，所以主线程并没有在此等待
						// 所以github评论中有人提到task没有并发执行是错误的看法
						task();
						_idlThrNum++;
					}
					});	// emplace_back结束
				_idlThrNum++;
			}
		}
	};
}

