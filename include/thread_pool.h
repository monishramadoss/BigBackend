#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include "types.h"

class Thread
{
private:
	bool destroying = false;
	std::thread worker;
	std::queue<std::function<void()>> jobQueue;
	std::mutex queueMutex;
	std::condition_variable condition;
	void queueLoop();
public:
	Thread() { worker = std::thread(&Thread::queueLoop, this); }
	~Thread();
	void addJob(std::function<void()>);
	void wait();
};

//TODO if mpi is working pause threadpool
class ThreadPool
{

public:
	_int num_threads{};
	std::vector<std::unique_ptr<Thread>> threads;
	void setThreadCount(_int count);
	void wait() const;
};
