#version 460

#include "common.h"
#include "update.h"

layout (push_constant) uniform PushConsts {
    UpdatePushConsts pushConstants;
};

layout (local_size_x = 32, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba8) writeonly uniform image2D outImage;

layout(std430, binding = 1) buffer AgentBuffer {
    Agent agents[];
};

void main() {
    // Get the global pixel coordinate for this invocation
    uint agent = gl_GlobalInvocationID.x;
    if (agent >= pushConstants.agentCount) {
        return; // Out of bounds, skip this invocation
    }

    Agent a = agents[agent];
    ivec2 pixel = ivec2(a.position);
    ivec2 imageSize = imageSize(outImage);

    if (pixel.x < 0 || pixel.x >= imageSize.x || pixel.y < 0 || pixel.y >= imageSize.y) {
        return; // Skip if the pixel is out of bounds
    }

    vec4 color = vec4(1, 1, 1, 1);
    imageStore(outImage, pixel, color);
}