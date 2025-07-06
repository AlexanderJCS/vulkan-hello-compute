#version 460

#include "common.h"

layout (push_constant) uniform PushConsts {
    ComputePushConsts pushConstants;
};

layout (local_size_x = 32, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba8) writeonly uniform image2D outImage;

void main() {
    // Get the global pixel coordinate for this invocation
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outImage);

    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y) {
        return;
    }

    imageStore(outImage, pixelCoord, vec4(1, 1, 1, 1));
}