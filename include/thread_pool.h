#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <map>
#include <vector>
#include "types.h"

class Thread
{
private:
	bool destroying = false;
	std::thread worker;
	std::queue<std::function<void()>> jobQueue;
	std::mutex queueMutex;
	std::condition_variable condition;
	void queue_loop();

public:
	Thread() { worker = std::thread(&Thread::queue_loop, this); }
	~Thread();
	void add_job(std::function<void()>);
	void wait();
	[[nodiscard]] std::thread::id pid() const { return worker.get_id(); }
};

//TODO if mpi is working pause threadpool
class ThreadPool
{

public:
	_int num_threads{};
	std::vector<std::thread::id> thread_ids;
	std::map<std::thread::id, std::unique_ptr<Thread>> threads;
	std::thread::id add_job(const std::function<void()>&) ;
	void setThreadCount(_int count);
	void wait() ;
	void wait(const std::thread::id);
};
