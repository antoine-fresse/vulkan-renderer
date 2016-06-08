#pragma once
#include <atomic>
#include <condition_variable>
#include <thread/fiber.h>
#include <vector>
#include <concurrentqueue/blockingconcurrentqueue.h>

/*
Fiber types : 

fiber pool : worker loop, pop a task, run it and decrease its counter
fiber_switching_fibers : loop : enqueue origin fiber (from tls) back in fiber pool then switch to target fiber (from tls) (use when restoring a waiting fiber in worker loop)
waiting_counter_fiber : loop : enqueue origin fiber (from tls) in waiting fiber map then switch to target fiber (from tls, one of the worker from pool)

*/

#define TASK_FUNC(name) void name(void *user_args, uint32_t bundle_index, uint32_t bundle_size)
#define TASK_FUNC_PTR(name) void(*name)(void*, uint32_t, uint32_t)

namespace kth
{

	
	typedef std::atomic<int> AtomicCounter;

	struct Task
	{
		Task(){}
		Task(TASK_FUNC_PTR(func), void* user_args, std::shared_ptr<AtomicCounter> counter, uint32_t bundle_index, uint32_t bundle_size) : function(func), user_args(user_args), counter(counter), bundle_index(bundle_index), bundle_size(bundle_size){}

		TASK_FUNC_PTR(function) = nullptr;
		void* user_args = nullptr;
		std::shared_ptr<AtomicCounter> counter;
		uint32_t bundle_index = 0;
		uint32_t bundle_size = 0;
	};

	struct WaitingTask
	{
		WaitingTask() : counter(nullptr), target(0), affinity(-1){}
		WaitingTask(Fiber fiber, std::shared_ptr<AtomicCounter> counter, int target, int affinity) : fiber(fiber), counter(counter), target(target), affinity(affinity) {}

		Fiber fiber;
		std::shared_ptr<AtomicCounter> counter;
		int target;
		int affinity;
	};

	class Multitasker
	{
	public:

		Multitasker(uint32_t worker_threads, uint32_t fiber_pool_size, std::function<void(uint32_t)> init_func);
		
		void fiber_switching_fiber_routine();
		void waiting_counter_fiber_routine();
		void process_tasks();

		std::shared_ptr<AtomicCounter> enqueue(TASK_FUNC_PTR(func), void* user_args);
		template<typename T, int N>
		std::shared_ptr<AtomicCounter> enqueue(TASK_FUNC_PTR(func), T(&user_args_array_fixed)[N]);
		template<typename T>
		std::shared_ptr<AtomicCounter> enqueue(TASK_FUNC_PTR(func), T* user_args_array_dynamic, int n);



		void wait_for(std::shared_ptr<AtomicCounter> counter, int value, bool return_on_same_thread = false);

		void stop() { _stop = true; _worker_wake_up.notify_all(); }
		static uint32_t get_current_thread_id() { return thread_id; }
	private:

		void init_worker(uint32_t worker_id);

		static thread_local uint32_t thread_id;
		std::recursive_mutex _mutex;
		std::vector<Fiber> _fiber_switching_fibers;
		static thread_local Fiber _fiber_switching_fiber_origin;
		static thread_local Fiber _fiber_switching_fiber_destination;

		std::vector<Fiber> _waiting_counter_fibers;
		static thread_local Fiber _waiting_counter_fiber_origin;
		static thread_local Fiber _waiting_counter_fiber_destination;
		static thread_local std::shared_ptr<AtomicCounter> _waiting_counter_fiber_counter;
		static thread_local int _waiting_counter_fiber_counter_target;
		static thread_local int _waiting_counter_fiber_affinity;

		moodycamel::BlockingConcurrentQueue<Fiber> _fiber_pool;
		std::vector<WaitingTask> _waiting_tasks;
		moodycamel::ConcurrentQueue<Task> _task_queue;

		std::atomic<bool> _stop;

		std::mutex _cv_mutex;
		std::condition_variable _worker_wake_up;

		std::function<void(uint32_t)> _init_function;

	};

	inline std::shared_ptr<AtomicCounter> Multitasker::enqueue(TASK_FUNC_PTR(func), void* user_args)
	{ 
		std::shared_ptr<AtomicCounter> counter = std::make_shared<AtomicCounter>(1);
		_task_queue.enqueue(Task(func, user_args, counter, 0, 1));
		_worker_wake_up.notify_one();
		return counter;
	}

	template<typename T, int N>
	std::shared_ptr<AtomicCounter> Multitasker::enqueue(TASK_FUNC_PTR(func), T(&user_args_array_fixed)[N])
	{
		return enqueue(func, user_args_array_fixed, N);
	}

	template<typename T>
	std::shared_ptr<AtomicCounter> Multitasker::enqueue(TASK_FUNC_PTR(func), T* user_args_array_dynamic, int n)
	{
		std::shared_ptr<AtomicCounter> counter = std::make_shared<AtomicCounter>(n);

		for (int i = 0; i < n; ++i)
		{
			_task_queue.enqueue(Task(func, user_args_array_dynamic + i, counter, i, n));
			_worker_wake_up.notify_one();
		}
		return counter;
	}

}
