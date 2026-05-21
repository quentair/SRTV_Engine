#include "buffer.h"

#include "error_checker.h"

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

VkBufferCreateInfo stagingBufferCreateInfo(size_t allocSize)
{
	// buffer creation infos
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;

	bufferCreateInfo.size = allocSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	return bufferCreateInfo;
}

void copyBuffer(VkBuffer srcBuffer, VmaAllocationInfo srcAllocInfo, VkBuffer dstBuffer, VmaAllocationInfo dstAllocInfo, VkQueue queue, VkCommandBuffer commandBuffer)
{
	// reset the command buffer to begin recording again
	CHECK_VK_ERROR(vkResetCommandBuffer(commandBuffer, 0));

	// we will use this command buffer only once, so we set this flag in the creation info
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// begin command buffer recording
	CHECK_VK_ERROR(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcAllocInfo.offset;
	copyRegion.dstOffset = dstAllocInfo.offset;
	copyRegion.size = srcAllocInfo.size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
}

} // namespace srtv_engine::buffer