#include "thread_pool.h"



void Thread::queueLoop()
{
	while (true)
	{
		std::function<void()> job;
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			condition.wait(lock, [this] {return !jobQueue.empty() || destroying; });
			if (destroying)
				break;
			job = jobQueue.front();
		}
		job();
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			jobQueue.pop();
			condition.notify_one();
		}
	}
}

void Thread::addJob(std::function<void()> function)
{
	std::lock_guard<std::mutex> lock(queueMutex);
	jobQueue.push(std::move(function));
	condition.notify_one();
}

void Thread::wait()
{
	std::unique_lock<std::mutex> lock(queueMutex);
	condition.wait(lock, [this]() { return jobQueue.empty(); });
}

Thread::~Thread()
{
	if (worker.joinable())
	{
		wait();
		queueMutex.lock();
		destroying = true;
		condition.notify_one();
		queueMutex.unlock();
		worker.join();
	}
}

void ThreadPool::setThreadCount(_int count)
{
	if (num_threads != 0)
		return;
	num_threads = count;
	threads.clear();
	for (_int i = 0; i < count; ++i)
		threads.push_back(std::make_unique<Thread>());
}

void ThreadPool::wait() const
{
	for (auto& thread : threads)
		thread->wait();
}
