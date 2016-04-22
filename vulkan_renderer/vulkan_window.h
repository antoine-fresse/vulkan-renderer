#pragma once

#include "vulkan_include.h"
#include <vector>

class vulkan_renderer;

class vulkan_window
{
public:
	vulkan_window();
	~vulkan_window();

	

	void									create(vulkan_renderer* renderer, uint32_t width, uint32_t height, uint32_t buffering = 3);
	void									destroy();

	

	void									window_size_changed();

private:
	
	

	

};