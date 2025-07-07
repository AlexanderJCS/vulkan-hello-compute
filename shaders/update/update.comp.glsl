#version 460

#include "common.h"
#include "update.h"

#extension GL_EXT_debug_printf : enable

layout (push_constant) uniform PushConsts {
    UpdatePushConsts pushConstants;
};

layout (local_size_x = 32, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba8) readonly uniform image2D readImage;
layout(binding = 1, rgba8) writeonly uniform image2D writeImage;

layout(std430, binding = 2) buffer AgentBuffer {
    Agent agents[];
};

const float SPEED = 30.0;

void main() {
    debugPrintfEXT("Thread %d is alive\n");

    if (pushConstants.deltaTime <= 0.0) {
        return; // Skip processing if deltaTime is zero or negative
    }

    // Get the global pixel coordinate for this invocation
    uint agentID = gl_GlobalInvocationID.x;
    if (agentID >= pushConstants.agentCount) {
        return; // Out of bounds, skip this invocation
    }

    Agent agent = agents[agentID];
    vec2 vel = vec2(cos(agent.angle), sin(agent.angle)) * SPEED * pushConstants.deltaTime;
    agent.position += vel;
    agent.position = mod(agent.position, vec2(imageSize(readImage))); // Wrap around the image size

    agents[agentID] = agent; // Update the agent in the buffer

    ivec2 pixel = ivec2(agent.position);
    ivec2 imageSize = imageSize(writeImage);

    if (pixel.x < 0 || pixel.x >= imageSize.x || pixel.y < 0 || pixel.y >= imageSize.y) {
        return; // Skip if the pixel is out of bounds
    }

    vec4 color = vec4(1, 1, 1, 1);
    imageStore(writeImage, pixel, color);
}