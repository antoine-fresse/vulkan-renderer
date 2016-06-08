#pragma once

#include <cstdint>

namespace std
{
	namespace this_thread
	{
		void set_affinity(uint32_t core);
	}
}

namespace kth
{
	uint32_t get_processor_count();
}