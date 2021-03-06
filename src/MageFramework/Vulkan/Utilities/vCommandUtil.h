#pragma once
#include <global.h>

namespace VulkanCommandUtil
{
	inline void copyCommandBuffer(VkDevice& logicalDevice, VkCommandBuffer& cmdBuffer,
		VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;

		vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}

	inline void createCommandPool(VkDevice& logicalDevice, VkCommandPool& cmdPool, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0)
	{
		// Command pools manage the memory that is used to store the buffers and command buffers are allocated from them.

		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
		cmdPoolCreateInfo.flags = flags; // Optional

		if (vkCreateCommandPool(logicalDevice, &cmdPoolCreateInfo, nullptr, &cmdPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool");
		}
	}

	inline void allocateCommandBuffers(VkDevice& logicalDevice, VkCommandPool& cmdPool, uint32_t cmdBufferCount, VkCommandBuffer* cmdBufferPointer,
		VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		// Specify the command pool and number of buffers to allocate

		// The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
		// VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Cannot be submitted directly, but can be called from primary command buffers.

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cmdPool;
		allocInfo.level = level;
		allocInfo.commandBufferCount = cmdBufferCount;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &allocInfo, cmdBufferPointer));
	}

	inline void allocateCommandBuffers(VkDevice& logicalDevice, VkCommandPool& cmdPool, std::vector<VkCommandBuffer>& cmdBuffers)
	{
		allocateCommandBuffers(logicalDevice, cmdPool, (uint32_t)cmdBuffers.size(), cmdBuffers.data());
	}

	inline VkCommandBuffer createCommandBuffer(VkDevice& logicalDevice, VkCommandPool& cmdPool,
		VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false,
		VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM)
	{
		VkCommandBuffer cmdBuffer;
		allocateCommandBuffers(logicalDevice, cmdPool, 1, &cmdBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		
		if (begin)
		{
			// Begin Single Command Buffer
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = flags;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to begin recording command buffer!");
			}
		}

		return cmdBuffer;
	}

	inline void beginCommandBuffer(VkCommandBuffer& cmdBuffer)
	{
		// Begin recording a command buffer by calling vkBeginCommandBuffer

		// The flags parameter specifies how we're going to use the command buffer. The following values are available:
		// - VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
		// - VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary command buffer that will be entirely within a single render pass.
		// - VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : The command buffer can be resubmitted while it is also already pending execution.

		// We use VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT because we may already be scheduling the drawing commands for the next frame 
		// while the last frame is not finished yet.

		// The pInheritanceInfo parameter is only relevant for secondary command buffers. 
		// It specifies which state to inherit from the calling primary command buffers.

		// If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly reset it. 
		// It's not possible to append commands to a buffer at a later time.

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}
	}

	inline void endCommandBuffer(VkCommandBuffer& cmdBuffer)
	{
		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	inline void submitToQueueSynced(VkQueue queue, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers,
		uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores, const VkPipelineStageFlags* pWaitDstStageMask,
		uint32_t signalSemaphoreCount, const VkSemaphore* pSignalSemaphores, VkFence inFlightFence)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = commandBufferCount;
		submitInfo.pCommandBuffers = pCommandBuffers;

		// These parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait
		// Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores
		submitInfo.waitSemaphoreCount = waitSemaphoreCount;
		submitInfo.pWaitSemaphores = pWaitSemaphores;
		submitInfo.pWaitDstStageMask = pWaitDstStageMask;
		submitInfo.signalSemaphoreCount = signalSemaphoreCount;
		submitInfo.pSignalSemaphores = pSignalSemaphores;

		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, inFlightFence));
	}

	inline void submitToQueue(uint32_t cmdBufferCount, const VkCommandBuffer& cmdBuffer, VkQueue& queue, VkDevice& logicalDevice)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = cmdBufferCount;
		submitInfo.pCommandBuffers = &cmdBuffer;

		// Unlike the draw commands, there are no events we need to wait on this time. 
		// We just want to execute the transfer on the buffers immediately. 
		// There are again two possible ways to wait on this transfer to complete. 
		// We could use a fence and wait with vkWaitForFences, or simply wait for the transfer queue to become idle with vkQueueWaitIdle. 
		// A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete, 
		// instead of executing one at a time. That may give the driver more opportunities to optimize.

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 0 };
		VkFence fence;
		vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence);

		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));

		// Wait for the fence to signal that command buffer has finished executing
		vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
		vkDestroyFence(logicalDevice, fence, nullptr);
		vkQueueWaitIdle(queue);
	}

	inline void beginSingleTimeCommand(VkDevice& logicalDevice, VkCommandPool& cmdPool, VkCommandBuffer& cmdBuffer)
	{
		VulkanCommandUtil::allocateCommandBuffers(logicalDevice, cmdPool, 1, &cmdBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		// Begin Single Command Buffer
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}
	}

	inline void endAndSubmitSingleTimeCommand(VkDevice& logicalDevice, VkQueue& queue, VkCommandPool& cmdPool, VkCommandBuffer& cmdBuffer)
	{
		endCommandBuffer(cmdBuffer);
		submitToQueue(1, cmdBuffer, queue, logicalDevice);
		vkFreeCommandBuffers(logicalDevice, cmdPool, 1, &cmdBuffer);
	}

	inline void beginRenderPass(VkCommandBuffer& cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer,
		VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue* clearValue)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		// The render area defines where shader loads and stores will take place. The pixels outside this region will have undefined values. 
		// It should match the size of the attachments for best performance.
		renderPassInfo.renderArea = renderArea;
		renderPassInfo.clearValueCount = clearValueCount;
		renderPassInfo.pClearValues = clearValue;

		// The final parameter of the vkCmdBeginRenderPass command controls how the drawing commands within the render pass will be provided.
		// It can have one of two values:
		// - VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself 
		//								and no secondary command buffers will be executed.
		// - VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : The render pass commands will be executed from secondary command buffers.
		vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	inline void pipelineBarrier(VkCommandBuffer cmdBuffer,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
		uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
		uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
		uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
	{
		// A pipeline barrier inserts an execution dependency and a set of memory dependencies between 
		// a set of commands earlier in the command buffer and a set of commands later in the command buffer.
		// Reference: https://vulkan.lunarg.com/doc/view/1.0.30.0/linux/vkspec.chunked/ch06s05.html

		// All types of pipeline barriers are submitted using the same function.

		// srcStageMask ==> specifies which pipeline stage should happen before the barrier
		// dstStageMask ==> specifies which pipeline stage waits on the barrier
		// dependencyFlags ==> can be either 0 or VK_DEPENDENCY_BY_REGION_BIT, the latter turns the barrier into a per-region condition
		//					That means that the implementation is allowed to already begin reading from the parts of a resource that
		//					were written so far, for example.
		vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, dependencyFlags,
			bufferMemoryBarrierCount, pMemoryBarriers,
			bufferMemoryBarrierCount, pBufferMemoryBarriers,
			imageMemoryBarrierCount, pImageMemoryBarriers);
	}

	inline void blitImage(VkCommandBuffer cmdBuffer,
		VkImage srcImage, VkImageLayout srcImageLayout,
		VkImage dstImage, VkImageLayout dstImageLayout,
		const VkImageBlit& blit, VkFilter filter)
	{
		vkCmdBlitImage(cmdBuffer,
			srcImage, srcImageLayout,
			dstImage, dstImageLayout,
			1, &blit, filter);
	}
}