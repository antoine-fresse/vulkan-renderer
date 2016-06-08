#include <thread/thread.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>

namespace std
{
	namespace this_thread
	{
		void set_affinity(uint32_t core)
		{
			SetThreadAffinityMask(GetCurrentThread(), 1<<core);
		}
	}
}


namespace kth
{
	uint32_t get_processor_count()
	{
		return GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);
	}
}