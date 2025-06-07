#ifndef RAYGUN_VK_SHADER_H
#define RAYGUN_VK_SHADER_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>


namespace raymarcher::graphics {
    class Shader {
    public:
        Shader() = default;
        Shader(VkDevice logicalDevice, const std::string& path, VkShaderStageFlagBits shaderStage, std::string entryPoint = "main");

        void destroy(VkDevice logicalDevice);

        [[nodiscard]] VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo() const;

    private:
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        VkShaderStageFlagBits shaderStage = static_cast<VkShaderStageFlagBits>(0);
        std::string entryPoint;

        static std::vector<char> readFile(const std::string& filepath);
        static VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& code);
    };
}


#endif //RAYGUN_VK_SHADER_H
