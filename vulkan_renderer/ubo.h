#pragma once

#include "vulkan_include.h"
#include "renderer.h"

template<typename T, bool keep_mapped=false>
class single_ubo
{

public:
	single_ubo(const renderer& renderer, const vk::BufferUsageFlags& usage) : _renderer(renderer)
	{
		vk::DeviceSize size = sizeof(T);
		vk::Device device = renderer.device();
		vk::BufferCreateInfo buffer_ci{ {}, size, usage, vk::SharingMode::eExclusive, 0, nullptr };
		_buffer = device.createBuffer(buffer_ci);
		_memory_reqs = device.getBufferMemoryRequirements(_buffer);

		vk::MemoryAllocateInfo mem_allocate_info{ _memory_reqs.size(), _renderer.find_adequate_memory(_memory_reqs, vk::MemoryPropertyFlagBits::eHostVisible) };
		_memory = device.allocateMemory(mem_allocate_info);

		device.bindBufferMemory(_buffer, _memory, 0);
	}

	single_ubo(const renderer& renderer, const vk::BufferUsageFlags& usage, const T& value) : single_ubo(renderer, usage)
	{
		update(value);
	}

	void update(const T& value)
	{
		if(!_mapped_value)
			_mapped_value = (T*)_renderer.device().mapMemory(_memory, 0, _memory_reqs.size(), {});

		*_mapped_value = value;

		if(!keep_mapped)
		{
			_renderer.device().unmapMemory(_memory);
			_mapped_value = nullptr;
		}
	}

	~single_ubo()
	{
		if(_mapped_value)
			_renderer.device().unmapMemory(_memory);

		_renderer.device().destroyBuffer(_buffer);
		_renderer.device().freeMemory(_memory);
	}
	
	const vk::Buffer& buffer() const { return _buffer; }

private:
	const renderer& _renderer;
	vk::DeviceMemory _memory;
	vk::Buffer _buffer;
	vk::MemoryRequirements _memory_reqs;
	T* _mapped_value = nullptr;
};