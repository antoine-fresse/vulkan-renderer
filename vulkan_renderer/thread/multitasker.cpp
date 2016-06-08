#include <thread/multitasker.h>
#include <thread/thread.h>


namespace
{
	FIBER_FUNC(process_task_fiber)
	{
		static_cast<kth::Multitasker*>(user_args)->process_tasks();
	}

	FIBER_FUNC(fiber_switching_fiber_routine_fiber)
	{
		static_cast<kth::Multitasker*>(user_args)->fiber_switching_fiber_routine();
	}

	FIBER_FUNC(waiting_counter_fiber_routine_fiber)
	{
		static_cast<kth::Multitasker*>(user_args)->waiting_counter_fiber_routine();
	}

}

namespace kth
{

	thread_local uint32_t Multitasker::thread_id = -1;
	thread_local Fiber Multitasker::_fiber_switching_fiber_origin(nullptr);
	thread_local Fiber Multitasker::_fiber_switching_fiber_destination(nullptr);
	thread_local Fiber Multitasker::_waiting_counter_fiber_origin(nullptr);
	thread_local Fiber Multitasker::_waiting_counter_fiber_destination(nullptr);
	thread_local std::shared_ptr<AtomicCounter> Multitasker::_waiting_counter_fiber_counter(nullptr);
	thread_local int Multitasker::_waiting_counter_fiber_counter_target(0);
	thread_local int Multitasker::_waiting_counter_fiber_affinity(0);
	

	Multitasker::Multitasker(uint32_t worker_threads, uint32_t fiber_pool_size, std::function<void(uint32_t)> init_func)
	{
		_stop = false;
		for (uint32_t i = 0; i < fiber_pool_size; ++i)
		{
			_fiber_pool.enqueue(create_fiber(process_task_fiber, this));
		}
		
		std::this_thread::set_affinity(0);
		convert_thread_to_fiber();
		thread_id = 0;
		_fiber_switching_fibers.resize(worker_threads);
		_waiting_counter_fibers.resize(worker_threads);

		_fiber_switching_fibers[thread_id] = create_fiber(fiber_switching_fiber_routine_fiber, this);
		_waiting_counter_fibers[thread_id] = create_fiber(waiting_counter_fiber_routine_fiber, this);

		_init_function = init_func;

		for (uint32_t i = 1; i < worker_threads; ++i)
		{
			std::thread(std::bind(&Multitasker::init_worker, this, i)).detach();
		}

		_init_function(0);
	}

	void Multitasker::init_worker(uint32_t worker_id)
	{
		thread_id = worker_id;
		convert_thread_to_fiber();
		std::this_thread::set_affinity(worker_id);

		{
			// std::lock_guard<std::recursive_mutex> lock(_mutex);
			_fiber_switching_fibers[thread_id] = create_fiber(fiber_switching_fiber_routine_fiber, this);
			_waiting_counter_fibers[thread_id] = create_fiber(waiting_counter_fiber_routine_fiber, this);
		}
		
		// Callback init function
		_init_function(worker_id);
		
		process_tasks();
	}

	void Multitasker::fiber_switching_fiber_routine()
	{
		for (;;)
		{
			_fiber_pool.enqueue(_fiber_switching_fiber_origin);
			switch_to_fiber(_fiber_switching_fiber_destination);
		}
	}

	void Multitasker::waiting_counter_fiber_routine()
	{
		for (;;)
		{
			
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_waiting_tasks.emplace_back(_waiting_counter_fiber_origin, _waiting_counter_fiber_counter, _waiting_counter_fiber_counter_target, _waiting_counter_fiber_affinity);
			}
			switch_to_fiber(_waiting_counter_fiber_destination);
		}

	}

	void Multitasker::process_tasks()
	{
		uint32_t id = thread_id;
		while (!_stop.load())
		{
			bool has_waiting_task = false;
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				if (_waiting_tasks.size())
				{
					for (auto it = _waiting_tasks.begin(); it != _waiting_tasks.end(); ++it)
					{
						if (it->counter->load() == it->target)
						{
							if(it->affinity == -1 || it->affinity == id)
							{
								_fiber_switching_fiber_destination = it->fiber;
								_fiber_switching_fiber_origin = get_current_fiber();

								if (it != --_waiting_tasks.end())
									*it = std::move(_waiting_tasks.back());
								_waiting_tasks.pop_back();
								has_waiting_task = true;
								break;
							}
						}
					}
				}
			}
			if(has_waiting_task)
			{
				switch_to_fiber(_fiber_switching_fibers[id]);
			}
			else
			{
				Task task;
				if (_task_queue.try_dequeue(task))
				{
					task.function(task.user_args, task.bundle_index, task.bundle_size);
					task.counter->fetch_sub(1); 
					// Wake all dormant workers, to check for waiting tasks
					_worker_wake_up.notify_all();
				}
				else
				{
					// Nothing to do, sleep a little
					// TODO(antoine): wait forever until signaled ? Need to check all possibles cases
					std::unique_lock<std::mutex> lock(_cv_mutex);
					_worker_wake_up.wait_for(lock, std::chrono::milliseconds(4));
				}
			}
			
		}


	}

	void Multitasker::wait_for(std::shared_ptr<AtomicCounter> counter, int value, bool return_on_same_thread)
	{
		if (counter->load() == value) return;

		auto id = thread_id;

		Fiber fiber;
		_fiber_pool.wait_dequeue(fiber);
		_waiting_counter_fiber_destination = fiber;
		_waiting_counter_fiber_origin = get_current_fiber();
		_waiting_counter_fiber_counter = counter;
		_waiting_counter_fiber_counter_target = value;
		_waiting_counter_fiber_affinity = return_on_same_thread ? thread_id : -1;
		switch_to_fiber(_waiting_counter_fibers[id]);
	}
}
