#pragma once
#include <stdexcept>

class renderer_exception : public std::runtime_error
{
public:
	explicit renderer_exception(const std::string& _Message)
		: runtime_error(_Message)
	{
	}

	explicit renderer_exception(const char* _Message)
		: runtime_error(_Message)
	{
	}
};


template<typename T, size_t N>
size_t array_sizeof(T(&)[N])
{
	return N;
}


struct input_state
{
	bool up;
	bool down;
	bool left;
	bool right;
};

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600