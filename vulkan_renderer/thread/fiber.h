#pragma once

#include <functional>

namespace kth
{

	#define FIBER_FUNC(name) void name(void* user_args)
	#define FIBER_FUNC_PTR(name) void(*name)(void*)


	struct Fiber
	{
		Fiber() : address(nullptr){}
		Fiber(void* address) : address(address) {}
		void* address;
	};

	Fiber create_fiber(FIBER_FUNC_PTR(func), void* user_args);

	Fiber convert_thread_to_fiber();
	void switch_to_fiber(Fiber& fiber);
	void delete_fiber(Fiber& fiber);
	Fiber get_current_fiber();
	

}