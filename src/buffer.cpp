#include "buffer.h"

namespace srtv_engine::buffer{

VkBufferCreateInfo defaultBufferCreateInfo(size_t allocSize, VkBufferUsageFlags usage)
{
	// buffer creation infos
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;

	bufferCreateInfo.size = allocSize;
	bufferCreateInfo.usage = usage;

	return bufferCreateInfo;
}

} // namespace srtv_engine::buffer