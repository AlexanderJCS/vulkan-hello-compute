#ifndef RAYMARCH_H
#define RAYMARCH_H

#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>

#include "tools/vktools.h"
#include "core/Buffer.h"
#include "core/CmdBuffer.h"
#include "window/Window.h"
#include "graphics/Camera.h"
#include "tools/Clock.h"

#include "../polyglot/common.h"
#include "../polyglot/update.h"

class Raymarcher {
public:
    Raymarcher();
    void renderLoop();
    ~Raymarcher();

private:
    void writeDescriptorSets();
    void runCompute();
    void draw(uint32_t& imageIndex);
    void present(uint32_t imageIndex);

    uint32_t renderWidth, renderHeight;
    int windowWidth, windowHeight;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    std::vector<raymarcher::graphics::Shader> shaders;
    raymarcher::core::PushConstants<ComputePushConsts> blurXPushConsts;
    raymarcher::core::PushConstants<ComputePushConsts> blurYPushConsts;
    raymarcher::core::PushConstants<UpdatePushConsts> updatePushConsts;
    raymarcher::graphics::Camera camera;
    raymarcher::window::Window renderWindow;
    VkInstance instance;
    VkDevice logicalDevice;
    std::vector<VkFramebuffer> framebuffers;
    raymarcher::graphics::Image pingImage;
    raymarcher::graphics::Image pongImage;

    raymarcher::graphics::Image* writeImage;
    raymarcher::graphics::Image* readImage;

    raymarcher::core::Buffer stagingBuffer;
    raymarcher::core::CmdBuffer cmdBuffer;
    raymarcher::core::DescriptorSet blurXDescriptorSet;
    raymarcher::core::DescriptorSet blurYDescriptorSet;
    raymarcher::core::DescriptorSet updateDescriptorSet;
    raymarcher::core::DescriptorSet rasterDescriptorSet;
    vktools::SyncObjects syncObjects;
    VkSampler fragmentImageSampler;
    VkRenderPass renderPass;
    vktools::PipelineInfo rasterPipeline;
    vktools::PipelineInfo blurXPipeline;
    vktools::PipelineInfo blurYPipeline;
    vktools::PipelineInfo updatePipeline;
    raymarcher::core::Buffer agentsBuffer;

    VkCommandPool commandPool;
    std::vector<VkImageView> swapchainImageViews;
    vktools::SwapchainObjects swapchainObjects;
    std::optional<VkDebugUtilsMessengerEXT> debugMessenger;
    VkSurfaceKHR surface;
};


#endif
