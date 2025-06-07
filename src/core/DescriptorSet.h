#ifndef RAYGUN_VK_DESCRIPTORSET_H
#define RAYGUN_VK_DESCRIPTORSET_H

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include "Buffer.h"
#include "../graphics/Image.h"

// forward declare
namespace vktools {
    struct AccStructureInfo;
}

namespace raymarcher::core {
    struct Binding {
        uint32_t bindingPoint;
        VkDescriptorType type;
        uint32_t descriptorCount;
        VkShaderStageFlagBits stageFlags;
        bool partiallyBound = false;

        [[nodiscard]] VkDescriptorSetLayoutBinding toLayoutBinding() const;
    };

    class DescriptorSet {
    public:
        DescriptorSet() = default;
        DescriptorSet(VkDevice logicalDevice, const std::vector<Binding>& bindings);

        void bind(VkCommandBuffer cmdBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout pipelineLayout);

        void writeBinding(VkDevice logicalDevice, int bindingPoint, const raymarcher::core::Buffer& buffer);
        void writeBinding(VkDevice logicalDevice, int bindingPoint, const raymarcher::graphics::Image& image, VkImageLayout imageLayout, VkSampler sampler);
        void writeBinding(VkDevice logicalDevice, int bindingPoint, const std::vector<raymarcher::graphics::Image>& images, VkImageLayout imageLayout, VkSampler sampler);
        void writeBinding(VkDevice logicalDevice, int bindingPoint, const vktools::AccStructureInfo& accStruct);

        void destroy(VkDevice device);

        [[nodiscard]] VkDescriptorSetLayout getLayout() const;
        [[nodiscard]] VkDescriptorPool getPool() const;
        [[nodiscard]] VkDescriptorSet getDescriptorSet() const;

    private:
        std::vector<Binding> bindings{};
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        [[nodiscard]] static bool hasDuplicateBindingPoints(const std::vector<Binding>& bindings);

        void writeBinding(VkDevice logicalDevice, int bindingPoint, VkDescriptorImageInfo *imageInfo,
                          VkDescriptorBufferInfo *bufferInfo, void *next);
    };
}

#endif //RAYGUN_VK_DESCRIPTORSET_H