#pragma once
#include <memory>
#include <mutex>
template<typename T>
class threadsafe_queue	// ??? 智能指针的解引用
{
private:
	struct node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
	};
	std::mutex head_mutex;
	std::unique_ptr<node> head;
	std::mutex tail_mutex;
	node* tail;
	std::condition_variable data_cond;
public:
	// 队列为空时，头尾节点指针相同
	threadsafe_queue() :
		head(new node), tail(head.get()) {}
	threadsafe_queue(const threadsafe_queue& other) = delete;
	threadsafe_queue& operator=(const threadsafe_queue& other) = delete;

	node* get_tail() {
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		return tail;
	}

	std::unique_ptr<node> pop_head() {
		std::unique_ptr<node> old_head = std::move(head);
		head = std::move(old_head->next);
		return old_head;
	}

	// head.get() != get_tail() 即一直等待，有数据时候才可以获得head_lock
	std::unique_lock<std::mutex> wait_for_data() {
		std::unique_lock<std::mutex> head_lock(head_mutex);
		data_cond.wait(head_lock, [&] {return head.get() != get_tail();});
		return std::move(head_lock);
	}

	// 如果队列为空，将一直等待，直到有数据弹出
	std::unique_ptr<node> wait_pop_head() {
		std::unique_lock<std::mutex> head_lock(wait_and_pop());
		return pop_head();
	}

	// 重载版本，可赋值
	std::unique_ptr<node> wait_pop_head(T& value) {
		std::unique_lock<std::mutex> head_lock(wait_and_pop());
		value = std::move(*head->data); 
		return pop_head();
	}

	// 如果队列为空，直接返回空指针，不等待
	std::unique_ptr<node> try_pop_head() {
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail()) {
			return std::unique_ptr<node>();
		}
		return pop_head();
	}

	// 重载版本，可赋值
	std::unique_ptr<node> try_pop_head(T& value) {
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail()) {
			return std::unique_ptr<node>();
		}
		value = std::move(*head->data);
		return pop_head();
	}

public:

	std::shared_ptr<T> wait_and_pop() {
		std::unique_ptr<node> cosnt old_head = wait_pop_head();
		return old_head;
	}

	void wait_and_pop(T& value) {
		std::unique_ptr<node> cosnt old_head = wait_pop_head(value);
	}

	std::shared_ptr<T> try_pop(T& value) {
		std::unique_ptr<node> old_head = try_pop_head();
		return old_head ? old_head->data : std::shared_ptr<T>();
 	};

	std::shared_ptr<T> try_pop() {
		std::unique_ptr<node> old_head = try_pop_head(value);
		return old_head ? old_head->data : std::shared_ptr<T>();
	};

	void push(T new_value) {
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
		std::unique_lock<node> p(new node); // 将空指针放在队列尾，将队列尾原来的空指针指向new_data
		{
			std::lock_guard<std::mutex> tail_lock(tail_mutex);
			tail->data = new_data;
			node* const new_tail = p.get();
			tail->next = std::move(p);
			tail = new_tail;
		}
		data_cond.notify_one();
	}

	bool empty() {
		std::lock_guard<std::mutex> head_lock(head_mutex);
		return (head.get() == get_tail());
	}
};