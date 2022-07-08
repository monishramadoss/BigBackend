#include "thread_pool.h"

void Thread::queue_loop()
{
	while (true)
	{
		std::function<void()> job;
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			condition.wait(lock, [this] { return !jobQueue.empty() || destroying; });
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

void Thread::add_job(std::function<void()> function)
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
	//threads.clear();
	for (_int i = 0; i < count; ++i)
	{
		auto th = std::make_unique<Thread>();
		thread_ids.push_back(th->pid());
		threads[th->pid()] = std::move(th);

	}
}

std::thread::id ThreadPool::add_job(const std::function<void()>& function)
{
	threads[thread_ids[0]]->add_job(function);
	return thread_ids[0];
}

void ThreadPool::wait()
{
	for (const auto& id : thread_ids)
		wait(id);
}

void ThreadPool::wait(const std::thread::id id)
{
	threads[id]->wait();
}
