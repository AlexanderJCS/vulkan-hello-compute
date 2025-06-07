#include "CmdBuffer.h"

#include <stdexcept>

raymarcher::core::CmdBuffer::CmdBuffer(VkDevice logicalDevice, VkCommandPool cmdPool, bool oneTime, bool fenceCreateSignaled)
        : cmdBuffer(nullptr), fence(nullptr), oneTime(oneTime), createdCmdPool(cmdPool) {
    // create command buffer
    VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = cmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &cmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    // begin command buffer
    begin();

    // create fence
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

    if (fenceCreateSignaled) {
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }

    if (vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fence");
    }
}

VkCommandBuffer raymarcher::core::CmdBuffer::getHandle() const {
    return cmdBuffer;
}

void raymarcher::core::CmdBuffer::endSubmit(VkDevice logicalDevice, VkQueue queue, const std::optional<VkSubmitInfo>& submitInfo) {
    // End command buffer
    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer");
    }

    // Reset fence
    if (vkResetFences(logicalDevice, 1, &fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset fence");
    }

    // Submit command buffer
    VkSubmitInfo queueSubmitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};

    if (submitInfo.has_value()) {
        queueSubmitInfo = submitInfo.value();

    }
    queueSubmitInfo.commandBufferCount = 1;
    queueSubmitInfo.pCommandBuffers = &cmdBuffer;

    if (vkQueueSubmit(queue, 1, &queueSubmitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit command buffer");
    }
}

void raymarcher::core::CmdBuffer::wait(VkDevice logicalDevice) {
    // they don't love you like I love you
    if (vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        throw std::runtime_error("Failed to wait for fence");
    }
}

void raymarcher::core::CmdBuffer::endWaitSubmit(VkDevice logicalDevice, VkQueue queue, const std::optional<VkSubmitInfo>& submitInfo) {
    endSubmit(logicalDevice, queue, submitInfo);
    wait(logicalDevice);
}

void raymarcher::core::CmdBuffer::destroy(VkDevice logicalDevice) {
    vkFreeCommandBuffers(logicalDevice, createdCmdPool, 1, &cmdBuffer);
    vkDestroyFence(logicalDevice, fence, nullptr);
}

void raymarcher::core::CmdBuffer::begin() {
    vkResetCommandBuffer(cmdBuffer, 0);  // reset before beginning

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = oneTime ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : static_cast<VkCommandBufferUsageFlags>(0)
    };

    if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Could not begin command buffer");
    }
}
