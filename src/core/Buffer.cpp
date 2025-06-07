#include <stdexcept>
#include "Buffer.h"

#include "../tools/vktools.h"

raymarcher::core::Buffer::Buffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize dataSize, VkBufferUsageFlags usage,
                                 VkMemoryAllocateFlags allocFlags, VkMemoryPropertyFlags memFlags): size(dataSize) {

    VkBufferCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = dataSize,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(logicalDevice, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

    VkMemoryAllocateFlagsInfo allocFlagsInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
            .flags = allocFlags
    };

    VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &allocFlagsInfo,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = vktools::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memFlags)
    };

    if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &deviceMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(logicalDevice, buffer, deviceMemory, 0);
}

VkBuffer raymarcher::core::Buffer::getHandle() const {
    return buffer;
}

VkDeviceMemory raymarcher::core::Buffer::getDeviceMemory() const {
    return deviceMemory;
}

VkDeviceAddress raymarcher::core::Buffer::getDeviceAddress(VkDevice logicalDevice) const {
    VkBufferDeviceAddressInfo addressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer
    };

    return vkGetBufferDeviceAddress(logicalDevice, &addressInfo);
}

void raymarcher::core::Buffer::destroy(VkDevice logicalDevice) {
    vkDestroyBuffer(logicalDevice, buffer, nullptr);
    vkFreeMemory(logicalDevice, deviceMemory, nullptr);
}

void raymarcher::core::Buffer::copyFrom(const raymarcher::core::CmdBuffer& cmdBuffer, const raymarcher::core::Buffer& src) {
    VkBufferCopy copyInfo{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = src.getSize()
    };

    vkCmdCopyBuffer(cmdBuffer.getHandle(), src.getHandle(), getHandle(), 1, &copyInfo);
}

VkDeviceSize raymarcher::core::Buffer::getSize() const {
    return size;
}
