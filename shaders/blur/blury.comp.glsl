#version 460

#include "common.h"
#include "blur.comp.glsl"

layout (push_constant) uniform PushConsts {
    ComputePushConsts pushConstants;
};

layout (local_size_x = 32, local_size_y = 8, local_size_z = 1) in;

void main() {
    float dt = pushConstants.deltaTime;
    dt = 1;
    if (dt <= 0.0) {
        return;
    }

    float sigma = 0.5;
    ivec2 pix   = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size  = imageSize(readImage);

    if (any(lessThan(pix, ivec2(0))) || any(greaterThanEqual(pix, size))) {
        return;
    }

    vec4 blurred = gaussianBlur1D(pix, size, sigma, ivec2(0, 1));
    imageStore(writeImage, pix, blurred);
}