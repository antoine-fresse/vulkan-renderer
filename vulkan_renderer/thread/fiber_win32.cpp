#include "fiber.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>



namespace kth
{

	Fiber create_fiber(FIBER_FUNC_PTR(func), void* user_args)
	{
		Fiber fiber;
		fiber.address = CreateFiber(0, (LPFIBER_START_ROUTINE)func, user_args);
		return fiber;
	}

	Fiber convert_thread_to_fiber()
	{
		Fiber fiber;
		fiber.address = ConvertThreadToFiber(nullptr);
		return fiber;
	}
	void switch_to_fiber(Fiber& fiber)
	{
		SwitchToFiber(fiber.address);
	}

	void delete_fiber(Fiber& fiber)
	{
		DeleteFiber(fiber.address);
	}
	Fiber get_current_fiber()
	{
		Fiber fiber;
		fiber.address = GetCurrentFiber();
		return fiber;
	}
}

