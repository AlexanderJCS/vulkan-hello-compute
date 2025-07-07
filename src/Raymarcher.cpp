#include "Raymarcher.h"

#include <vulkan/vulkan.h>

#include "graphics/Camera.h"

#include <stdexcept>
#include <iostream>


Raymarcher::Raymarcher() {
    // init
    renderWidth = 800;  // todo: bug - when renderWidth < windowWidth, the image appears stretched
    renderHeight = 800;
    const float aspectRatio = static_cast<float>(renderWidth) / static_cast<float>(renderHeight);

    windowWidth = 800;
    windowHeight = static_cast<int>(static_cast<float>(windowWidth) / aspectRatio);

    renderWindow = raymarcher::window::Window {windowWidth, windowHeight};

    instance = vktools::createInstance();
    debugMessenger = vktools::createDebugMessenger(instance);
    surface = vktools::createSurface(instance, renderWindow.getGlfwWindow());
    VkPhysicalDevice physicalDevice = vktools::pickPhysicalDevice(instance);
    logicalDevice = vktools::createLogicalDevice(surface, physicalDevice);

    vktools::QueueFamilyIndices indices = vktools::findQueueFamilies(surface, physicalDevice);
    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);

    swapchainObjects = vktools::createSwapchain(surface, physicalDevice, logicalDevice, renderWindow.getWidth(), renderWindow.getHeight());
    swapchainImageViews = vktools::createSwapchainImageViews(logicalDevice, swapchainObjects.swapchainImageFormat, swapchainObjects.swapchainImages);

    commandPool = vktools::createCommandPool(physicalDevice, logicalDevice, surface);

    cmdBuffer = raymarcher::core::CmdBuffer{logicalDevice, commandPool, false, true};
    cmdBuffer.endWaitSubmit(logicalDevice, graphicsQueue);  // since the command buffer automatically begins upon creation, and we don't want that in this specific case

    glm::vec3 pos = glm::vec3(-1.6899, 0.317017, 1.6386);
    glm::vec3 lookAt = glm::vec3(0, 0.962f, 0);
    camera = raymarcher::graphics::Camera{renderWindow, glm::radians(25.0f), aspectRatio, pos, glm::normalize(lookAt - pos)};

    pingImage = raymarcher::graphics::Image{
            logicalDevice, physicalDevice, renderWidth, renderHeight, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    pongImage = raymarcher::graphics::Image{
            logicalDevice, physicalDevice, renderWidth, renderHeight, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    writeImage = &pingImage;
    readImage = &pongImage;

    fragmentImageSampler = vktools::createSampler(logicalDevice);

    blurXPushConsts = raymarcher::core::PushConstants{
        ComputePushConsts{camera.getInverseView(), camera.getInverseProjection()},
        VK_SHADER_STAGE_COMPUTE_BIT
    };

    updatePushConsts = raymarcher::core::PushConstants{
            UpdatePushConsts{},
            VK_SHADER_STAGE_COMPUTE_BIT
    };

    updateDescriptorSet = raymarcher::core::DescriptorSet{
            logicalDevice,
            {
                    raymarcher::core::Binding{0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT},   // image from previous frame
                    raymarcher::core::Binding{1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT},   // image from previous frame
                    raymarcher::core::Binding{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT}  // agent positions
            }
    };

    blurXDescriptorSet = raymarcher::core::DescriptorSet{
            logicalDevice,
            {
                    raymarcher::core::Binding{0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                    raymarcher::core::Binding{1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT}
            }
    };
    raymarcher::graphics::Shader blurXShader{logicalDevice, "shaders/blur/blurx.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT};
    blurXPipeline = vktools::createComputePipeline(logicalDevice, blurXDescriptorSet, blurXShader, blurXPushConsts);
    blurXShader.destroy(logicalDevice);

    raymarcher::graphics::Shader updateShader{logicalDevice, "shaders/update/update.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT};
    updatePipeline = vktools::createComputePipeline(logicalDevice, updateDescriptorSet, updateShader, updatePushConsts);
    updateShader.destroy(logicalDevice);

    rasterDescriptorSet = raymarcher::core::DescriptorSet{
            logicalDevice,
            {
                    raymarcher::core::Binding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}
            }
    };

    raymarcher::graphics::Shader vertexShader = raymarcher::graphics::Shader(logicalDevice, "shaders/raster/display.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    raymarcher::graphics::Shader fragmentShader = raymarcher::graphics::Shader(logicalDevice, "shaders/raster/display.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    renderPass = vktools::createRenderPass(logicalDevice, swapchainObjects.swapchainImageFormat);
    rasterPipeline = vktools::createRasterizationPipeline(logicalDevice, rasterDescriptorSet, renderPass, vertexShader, fragmentShader);

    framebuffers = vktools::createSwapchainFramebuffers(logicalDevice, renderPass, swapchainObjects.swapchainExtent, swapchainImageViews);

    vertexShader.destroy(logicalDevice);
    fragmentShader.destroy(logicalDevice);

    syncObjects = vktools::createSyncObjects(logicalDevice);

    VkDeviceSize imageSize = renderWidth * renderHeight * 4;  // RGBA8

    stagingBuffer = raymarcher::core::Buffer{
            logicalDevice, physicalDevice, imageSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            static_cast<VkMemoryAllocateFlags>(0),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    std::vector<Agent> defaultAgents;
    defaultAgents.push_back(Agent{glm::vec2(400, 400), 0});

    agentsBuffer = raymarcher::core::Buffer{
        logicalDevice, physicalDevice, defaultAgents,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        static_cast<VkMemoryAllocateFlags>(0),
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  // TODO: make this non-host visible for max perf
    };

    writeDescriptorSets();
}


void Raymarcher::renderLoop() {
    VkCommandBuffer cmdBufferHandle = cmdBuffer.getHandle();

    raymarcher::tools::Clock clock;
    while (!renderWindow.shouldClose()) {
        updatePushConsts.getPushConstants().deltaTime = static_cast<float>(clock.getTimeDelta());
        blurXPushConsts.getPushConstants().deltaTime = static_cast<float>(clock.getTimeDelta());

        // render image
        cmdBuffer.wait(logicalDevice);
        cmdBuffer.begin();

        runCompute();

        // render
        readImage->transition(cmdBufferHandle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        uint32_t imageIndex = -1;
        if (!renderWindow.isMinimized()) {
            draw(imageIndex);
        }

        VkPipelineStageFlags waitStages[] = {
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
        };

        VkSubmitInfo submitInfo{
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = renderWindow.isMinimized() ? 0u : 1u,
                .pWaitSemaphores      = renderWindow.isMinimized() ? nullptr : &syncObjects.imageAvailableSemaphore,
                .pWaitDstStageMask    = waitStages,
                .commandBufferCount   = 1,
                .pCommandBuffers      = &cmdBufferHandle,
                .signalSemaphoreCount = renderWindow.isMinimized() ? 0u : 1u,
                .pSignalSemaphores    = renderWindow.isMinimized() ? nullptr : &syncObjects.renderFinishedSemaphore
        };

        cmdBuffer.endSubmit(logicalDevice, graphicsQueue, submitInfo);

        // Present the swapchain image
        if (!renderWindow.isMinimized()) {
            present(imageIndex);
        }

        glfwPollEvents();
        clock.markFrame();
    }

    vkDeviceWaitIdle(logicalDevice);
}

void Raymarcher::writeDescriptorSets() {
    updateDescriptorSet.writeBinding(logicalDevice, 2, agentsBuffer);
}

void Raymarcher::runCompute() {
    const int workgroupWidth = 32;
    const int workgroupHeight = 8;

    const int localSizeX = 256;

    readImage->transition(cmdBuffer.getHandle(), VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    writeImage->transition(cmdBuffer.getHandle(), VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    updateDescriptorSet.writeBinding(logicalDevice, 0, *readImage, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE);
    updateDescriptorSet.writeBinding(logicalDevice, 1, *writeImage, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE);
    updateDescriptorSet.bind(cmdBuffer.getHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, updatePipeline.pipelineLayout);

    std::swap(writeImage, readImage);

    updatePushConsts.getPushConstants().agentCount = static_cast<int>(agentsBuffer.getSize());

    updatePushConsts.push(cmdBuffer.getHandle(), updatePipeline.pipelineLayout);
    vkCmdBindPipeline(cmdBuffer.getHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, updatePipeline.pipeline);
    vkCmdDispatch(
            cmdBuffer.getHandle(),
            (agentsBuffer.getSize() + localSizeX - 1) / localSizeX,
            1,
            1
    );

    vkCmdPipelineBarrier(
            cmdBuffer.getHandle(),
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            0, nullptr
    );

    readImage->transition(cmdBuffer.getHandle(), VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    writeImage->transition(cmdBuffer.getHandle(), VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    blurXDescriptorSet.writeBinding(logicalDevice, 0, *readImage, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE);
    blurXDescriptorSet.writeBinding(logicalDevice, 1, *writeImage, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE);

    blurXDescriptorSet.bind(cmdBuffer.getHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, blurXPipeline.pipelineLayout);
    blurXPushConsts.push(cmdBuffer.getHandle(), blurXPipeline.pipelineLayout);

    vkCmdBindPipeline(cmdBuffer.getHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, blurXPipeline.pipeline);
    vkCmdDispatch(
            cmdBuffer.getHandle(),
            (renderWidth + workgroupWidth - 1) / workgroupWidth,
            (renderHeight + workgroupHeight - 1) / workgroupHeight,
            1
    );

    vkCmdPipelineBarrier(
            cmdBuffer.getHandle(),
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            0, nullptr
            );

    std::swap(writeImage, readImage);
}

void Raymarcher::draw(uint32_t& imageIndex) {
    VkResult result = vkAcquireNextImageKHR(logicalDevice, swapchainObjects.swapchain, UINT64_MAX, syncObjects.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Swapchain is either out of date or suboptimal");
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    VkClearValue clearColor = {{0, 0, 0, 1}};

    VkRenderPassBeginInfo renderPassBeginInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = framebuffers[imageIndex],
            .renderArea = {
                    .offset = {0, 0},
                    .extent = swapchainObjects.swapchainExtent
            },
            .clearValueCount = 1,
            .pClearValues = &clearColor
    };

    vkCmdBeginRenderPass(cmdBuffer.getHandle(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    rasterDescriptorSet.writeBinding(logicalDevice,0, *readImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, fragmentImageSampler);

    rasterDescriptorSet.bind(cmdBuffer.getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, rasterPipeline.pipelineLayout);

    vkCmdBindPipeline(cmdBuffer.getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, rasterPipeline.pipeline);

    VkViewport viewport{
            .x = 0,
            .y = 0,
            .width = static_cast<float>(swapchainObjects.swapchainExtent.width),
            .height = static_cast<float>(swapchainObjects.swapchainExtent.height),
            .minDepth = 0,
            .maxDepth = 1
    };
    vkCmdSetViewport(cmdBuffer.getHandle(), 0, 1, &viewport);

    VkRect2D scissor{
            .offset = {0, 0},
            .extent = swapchainObjects.swapchainExtent
    };

    vkCmdSetScissor(cmdBuffer.getHandle(), 0, 1, &scissor);
    vkCmdDraw(cmdBuffer.getHandle(), 6, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuffer.getHandle());
}

void Raymarcher::present(uint32_t imageIndex) {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &syncObjects.renderFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainObjects.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);
}

Raymarcher::~Raymarcher() {
    for (VkFramebuffer framebuffer : framebuffers) {
        vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
    }

    pingImage.destroy(logicalDevice);
    pongImage.destroy(logicalDevice);

    stagingBuffer.destroy(logicalDevice);
    agentsBuffer.destroy(logicalDevice);

    vkDestroySampler(logicalDevice, fragmentImageSampler, nullptr);
    cmdBuffer.destroy(logicalDevice);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
    updateDescriptorSet.destroy(logicalDevice);
    blurXDescriptorSet.destroy(logicalDevice);
    rasterDescriptorSet.destroy(logicalDevice);
    vkDestroySemaphore(logicalDevice, syncObjects.renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(logicalDevice, syncObjects.imageAvailableSemaphore, nullptr);
    vkDestroyPipeline(logicalDevice, rasterPipeline.pipeline, nullptr);
    vkDestroyPipeline(logicalDevice, blurXPipeline.pipeline, nullptr);
    vkDestroyPipeline(logicalDevice, updatePipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, updatePipeline.pipelineLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, rasterPipeline.pipelineLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, blurXPipeline.pipelineLayout, nullptr);

    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

    for (VkImageView imageView : swapchainImageViews) {
        vkDestroyImageView(logicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(logicalDevice, swapchainObjects.swapchain, nullptr);

    vkDestroyDevice(logicalDevice, nullptr);

    if (debugMessenger.has_value()) {
        vktools::DestroyDebugUtilsMessengerEXT(instance, debugMessenger.value(), nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    renderWindow.destroy();
}
