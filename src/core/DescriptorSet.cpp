#include "DescriptorSet.h"

#include <stdexcept>

#include "../tools/vktools.h"

VkDescriptorSetLayoutBinding raymarcher::core::Binding::toLayoutBinding() const {
    return VkDescriptorSetLayoutBinding{
        .binding = bindingPoint,
        .descriptorType = type,
        .descriptorCount = descriptorCount,
        .stageFlags = static_cast<VkShaderStageFlags>(stageFlags),
        .pImmutableSamplers = nullptr
    };
}

raymarcher::core::DescriptorSet::DescriptorSet(VkDevice logicalDevice, const std::vector<Binding>& bindings)
        : bindings(bindings) {
    if (hasDuplicateBindingPoints(bindings)) {
        throw std::runtime_error("Cannot initialize descriptor set: duplicate binding points found");
    }

    // Create descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> vkBindings(bindings.size());
    std::vector<VkDescriptorBindingFlags> bindingFlags(bindings.size());

    for (size_t i = 0; i < bindings.size(); ++i) {
        vkBindings[i] = bindings[i].toLayoutBinding();

        if (bindings[i].partiallyBound) {
            bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;
        }
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(vkBindings.size()),
        .pBindingFlags = bindingFlags.data()
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &bindingFlagsInfo,
        .bindingCount = static_cast<uint32_t>(vkBindings.size()),
        .pBindings = vkBindings.data(),
    };

    if (vkCreateDescriptorSetLayout(logicalDevice, &layoutCreateInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("Cannot create descriptor set layout");
    }

    // Create descriptor pool (account for all descriptor types in bindings)
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const auto& binding : bindings) {
        bool typeExists = false;
        for (auto& poolSize : poolSizes) {
            if (poolSize.type == binding.type) {
                poolSize.descriptorCount += 1;
                typeExists = true;
                break;
            }
        }
        if (!typeExists) {
            poolSizes.push_back(VkDescriptorPoolSize{
                .type = binding.type,
                .descriptorCount = 1
            });
        }
    }

    VkDescriptorPoolCreateInfo poolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1, // Allocate 1 descriptor set
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    if (vkCreateDescriptorPool(logicalDevice, &poolCreateInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1, // Allocate 1 descriptor set
        .pSetLayouts = &layout   // Use the layout created earlier
    };

    if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set");
    }
}

void raymarcher::core::DescriptorSet::destroy(VkDevice logicalDevice) {
    if (layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(logicalDevice, layout, nullptr);
    }

    if (pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(logicalDevice, pool, nullptr);
    }
}

bool raymarcher::core::DescriptorSet::hasDuplicateBindingPoints(const std::vector<Binding>& bindings) {
    for (int i = 0; i < bindings.size(); i++) {
        for (int j = i + 1; j < bindings.size(); j++) {
            if (bindings[i].bindingPoint == bindings[j].bindingPoint) {
                return true;
            }
        }
    }

    return false;
}

VkDescriptorSetLayout raymarcher::core::DescriptorSet::getLayout() const {
    return layout;
}

VkDescriptorPool raymarcher::core::DescriptorSet::getPool() const {
    return pool;
}

VkDescriptorSet raymarcher::core::DescriptorSet::getDescriptorSet() const {
    return descriptorSet;
}

void raymarcher::core::DescriptorSet::bind(VkCommandBuffer cmdBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout pipelineLayout) {
    vkCmdBindDescriptorSets(
            cmdBuffer,
            bindPoint,
            pipelineLayout,
            0,
            1,
            &descriptorSet,
            0,
            nullptr
    );
}

void raymarcher::core::DescriptorSet::writeBinding(VkDevice logicalDevice, int bindingPoint, VkDescriptorImageInfo *imageInfo,
                                                   VkDescriptorBufferInfo *bufferInfo, void *next) {

    for (const Binding& binding : bindings) {
        if (binding.bindingPoint != bindingPoint) {
            continue;
        }

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding.bindingPoint;
        descriptorWrite.dstArrayElement = 0;  // assuming we are not working with arrays of descriptors per binding
        descriptorWrite.descriptorType = binding.type;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = imageInfo;
        descriptorWrite.pBufferInfo = bufferInfo;
        descriptorWrite.pTexelBufferView = nullptr;
        descriptorWrite.pNext = next;

        // Ensure valid VkSampler for sampler or combined image sampler descriptor types
        if ((binding.type == VK_DESCRIPTOR_TYPE_SAMPLER || binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) && (imageInfo == nullptr || imageInfo->sampler == VK_NULL_HANDLE)) {
            throw std::runtime_error("Invalid VkSampler for sampler or combined image sampler descriptor type");
        }

        vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
        break;
    }
}

void raymarcher::core::DescriptorSet::writeBinding(VkDevice logicalDevice, int bindingPoint, const raymarcher::core::Buffer& buffer) {
    VkDescriptorBufferInfo bufferInfo{.buffer = buffer.getHandle(), .offset = 0, .range = VK_WHOLE_SIZE};
    writeBinding(logicalDevice, bindingPoint, nullptr, &bufferInfo, nullptr);
}

void raymarcher::core::DescriptorSet::writeBinding(VkDevice logicalDevice, int bindingPoint, const raymarcher::graphics::Image& image, VkImageLayout imageLayout, VkSampler sampler) {
    VkDescriptorImageInfo imageInfo{.sampler = sampler, .imageView = image.getImageView(), .imageLayout = imageLayout};
    writeBinding(logicalDevice, bindingPoint, &imageInfo, nullptr, nullptr);
}

void raymarcher::core::DescriptorSet::writeBinding(VkDevice logicalDevice, int bindingPoint, const vktools::AccStructureInfo& accStruct) {
    VkWriteDescriptorSetAccelerationStructureKHR descriptorAccStructure{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
            .accelerationStructureCount = 1,
            .pAccelerationStructures = &accStruct.accelerationStructure
    };

    writeBinding(logicalDevice, bindingPoint, nullptr, nullptr, &descriptorAccStructure);
}

void raymarcher::core::DescriptorSet::writeBinding(VkDevice logicalDevice, int bindingPoint,
                                                   const std::vector<raymarcher::graphics::Image>& images,
                                                   VkImageLayout imageLayout, VkSampler sampler) {

    // todo: merge this with the generic writeBinding to prevent some repeated code
    auto imageCount = static_cast<uint32_t>(images.size());
    std::vector<VkDescriptorImageInfo> imageInfos(imageCount);

    for (uint32_t i = 0; i < imageCount; i++) {
        imageInfos[i].sampler = sampler;
        imageInfos[i].imageView = images[i].getImageView();
        imageInfos[i].imageLayout = imageLayout;
    }

    for (const Binding& binding : bindings) {
        if (binding.bindingPoint != bindingPoint) {
            continue;
        }

        VkWriteDescriptorSet descriptorWrite{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet,
                .dstBinding = binding.bindingPoint,
                .dstArrayElement = 0,
                .descriptorCount = imageCount,
                .descriptorType = binding.type,
                .pImageInfo = imageInfos.data()
        };

        vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
        break;
    }
}
