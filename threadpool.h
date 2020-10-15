/*
	�Ż�֮ǰ�Ĳο��̳߳�ʵ�֣��ڸ��ļ�������˸���ϸ��ע�ͣ�����������Щ�ط���Ҫ�Ż���
	ֻ�жԱȾ�ʵ�֣���֪��Ϊʲô��Ҫ��ʵ�֣�������ļ����ڵ����塣
	reference��https://github.com/lzpong/threadpool/blob/master/threadpool.h
	��Ҫ�Ż��Ĳ��֣���д�����̰߳�ȫ�Ķ���
	����ϸ��ע�Ͱ�����Ϊʲôʹ�ö�д����Ϊʲôʹ�ø�ϸ�����̰߳�ȫ�Ķ��С�
	�����߲������ܵ�С���ɡ���github���ۣ��ÿ�û�в������Ļش�
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
		using Task = function<void()>;	//��������
		vector<thread> _pool;     //�̳߳�
		threadsafe_queue<Task> _tasks;            //�������
		// ���ڶ�������̶߳�������У�������Ҫ��д����ʵ�֣���֤ͬһʱ�̿����ж���߳�ͬʱ��ȡ_tasks��
		// ��������߳���д��_tasks�������̶߳����ܶ�ȡ_tasks������������ѧϰ��д����
		mutex _lock;                   //ͬ��������������еĶ�дȨ�ޣ������
		condition_variable _task_cv;   //��������
		atomic<bool> _run{ true };     //�̳߳��Ƿ�ִ�У��������ݣ�
		atomic<int>  _idlThrNum{ 0 };  //�����߳��������������ݣ�

	public:
		inline threadpool(unsigned short size = 4) { addThread(size); }
		inline ~threadpool()
		{
			_run = false;
			_task_cv.notify_all(); // ���������߳�ִ��
			for (thread& thread : _pool) {
				//thread.detach(); // ���̡߳���������
				if (thread.joinable())
					thread.join(); // �ȴ���������� ǰ�᣺�߳�һ����ִ����
			}
		}

		// ���������task_������һ������ִ��
		template<class F, class... Args>
		auto commit(F&& f, Args&&... args) ->future<decltype(f(args...))>
		{
			if (!_run)    // stoped ??
				throw runtime_error("commit on ThreadPool is stopped.");

			using RetType = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, ���� f �ķ���ֵ����
			auto task = make_shared<packaged_task<RetType()>>(
				bind(forward<F>(f), forward<Args>(args)...)	// forward����ֹ��ʱ�������ֵ���ñ�������ֵ������
				); // �Ѻ�����ڼ�����,���(��)
			future<RetType> future = task->get_future(); // ����Ҳû��ִ������ע���future����get������Ҫ����
			{    // ������񵽶���
				lock_guard<mutex> lock{ _lock };//�Ե�ǰ���������  lock_guard �� mutex �� stack ��װ�࣬�����ʱ�� lock()��������ʱ�� unlock()
				_tasks.push([task]() { // push(Task{...}) �ŵ����к���
					(*task)();
					});
			}
#ifdef THREADPOOL_AUTO_GROW
			if (_idlThrNum < 1 && _pool.size() < THREADPOOL_MAX_NUM)
				addThread(1);
#endif // !THREADPOOL_AUTO_GROW
			_task_cv.notify_one(); // ����һ���߳�ִ�ж���ͷ�������񣨲��Ǳ���commit�����񣩣���������α���ѣ�

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
			{   //�����߳�����,�������� Ԥ�������� THREADPOOL_MAX_NUM
				_pool.emplace_back([this] { // ��ӹ����̣߳������̣߳�������ֱ�ӿ�ʼ���У�ֱ��_run == false
					while (_run)
					{
						// ��ȡһ����ִ�е� task
						Task task; // ����ס����Ԫ֮ǰ�����ڴ���䣬�������ݾ���
						{
							// unique_lock ��� lock_guard �ĺô��ǣ�������ʱ unlock() �� lock()
							unique_lock<mutex> lock{ _lock };	// ÿһ�������̷߳������������Ҫ��Ȩ��
							// ������������㣬�����unlock()���������̣߳����̣߳�
							_task_cv.wait(lock, [this] {
								return !_run || !_tasks.empty();
								}); // wait ֱ���� task
							if (!_run && _tasks.empty())
								return;
							task = *_tasks.wait_and_pop(); // �߳�����ֱ�������񵯳�
						}	// unlock()��������з������
						_idlThrNum--;
						// ִ�����񣬴˴����ڷ����߳�ִ�У��������̲߳�û���ڴ˵ȴ�
						// ����github�����������ᵽtaskû�в���ִ���Ǵ���Ŀ���
						task();
						_idlThrNum++;
					}
					});	// emplace_back����
				_idlThrNum++;
			}
		}
	};
}

