#ifndef RAYGUN_VK_VKTOOLS_H
#define RAYGUN_VK_VKTOOLS_H

#include <stdexcept>
#include <optional>
#include <utility>
#include <vector>
#include <string>
#include <memory>

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"

#include "../graphics/Shader.h"
#include "../core/DescriptorSet.h"
#include "../core/PushConstants.h"
#include "../core/Buffer.h"

namespace vktools {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool isComplete() const;
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct SwapchainObjects {
        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchainImages;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
    };

    struct SbtSpacing {
        VkDeviceSize headerSize;
        VkDeviceSize baseAlignment;
        VkDeviceSize handleAlignment;
        VkDeviceSize stride;
    };

    struct PipelineInfo {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };

    struct SyncObjects {
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
    };

    struct AccStructureInfo {
        VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
        raymarcher::core::Buffer buffer;
    };

    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool hasValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
    );

    VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    uint64_t getDeviceLocalMemory(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);

    template <typename T>
    void loadVkFunc(VkDevice logicalDevice, const char* funcName, T& funcPtr) {
        funcPtr = reinterpret_cast<T>(vkGetDeviceProcAddr(logicalDevice, funcName));
        if (!funcPtr) {
            throw std::runtime_error(std::string("Failed to load function: ") + funcName);
        }
    }

    template <typename T>
    PipelineInfo createComputePipeline(VkDevice logicalDevice, const::raymarcher::core::DescriptorSet& descriptorSet, const raymarcher::graphics::Shader& shader, const raymarcher::core::PushConstants<T>& pushConstants) {
        VkDescriptorSetLayout descriptorLayout = descriptorSet.getLayout();
        VkPushConstantRange range = pushConstants.getRange();
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = &descriptorLayout,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &range
        };

        VkPipelineLayout pipelineLayout;
        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout");
        }

        VkComputePipelineCreateInfo pipelineCreateInfo{
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .stage = shader.pipelineShaderStageCreateInfo(),
                .layout = pipelineLayout
        };

        VkPipeline computePipeline;
        if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &computePipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create compute pipeline");
        }

        return {computePipeline, pipelineLayout};
    }

    PipelineInfo createComputePipeline(VkDevice logicalDevice, const::raymarcher::core::DescriptorSet& descriptorSet, const raymarcher::graphics::Shader& shader);

    std::vector<VkFramebuffer> createSwapchainFramebuffers(VkDevice logicalDevice, VkRenderPass renderPass, VkExtent2D extent, const std::vector<VkImageView>& swapchainImageViews);
    PipelineInfo createRasterizationPipeline(VkDevice logicalDevice, const raymarcher::core::DescriptorSet& descriptorSet, VkRenderPass renderPass, const raymarcher::graphics::Shader& vertexShader, const raymarcher::graphics::Shader& fragmentShader);
    VkRenderPass createRenderPass(VkDevice logicalDevice, VkFormat swapchainImageFormat);

    SyncObjects createSyncObjects(VkDevice logicalDevice);
    VkSampler createSampler(VkDevice logicalDevice);

    VkCommandPool createCommandPool(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface);
    std::vector<VkImageView> createSwapchainImageViews(VkDevice logicalDevice, VkFormat swapchainImageFormat, std::vector<VkImage> swapchainImages);
    SwapchainObjects createSwapchain(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice logicalDevice, int windowWidth, int windowHeight);
    VkDevice createLogicalDevice(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
    VkPhysicalDevice pickPhysicalDevice(VkInstance instance);
    VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window);
    std::optional<VkDebugUtilsMessengerEXT> createDebugMessenger(VkInstance instance);
    VkInstance createInstance();
}


#endif //RAYGUN_VK_VKTOOLS_H
