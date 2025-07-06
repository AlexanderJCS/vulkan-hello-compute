#version 460

#include "common.h"

layout (push_constant) uniform PushConsts {
    ComputePushConsts pushConstants;
};

layout (local_size_x = 32, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba8) readonly uniform image2D readImage;
layout(binding = 1, rgba8) writeonly uniform image2D writeImage;

struct Ray {
    vec3 origin;
    vec3 direction;
};

Ray getStartingRay(
    vec2 pixel,
    vec2 resolution,
    mat4 invView,
    mat4 invProjection
) {
    vec2 pixelCenter = pixel + vec2(0.5);

    vec2 ndc = vec2(
        (pixelCenter.x / resolution.x) * 2.0 - 1.0,
        -((pixelCenter.y / resolution.y) * 2.0 - 1.0)  // Flip y-coordinate so image isn't upside down.
    );

    vec4 clipPos = vec4(ndc, -1.0, 1.0);

    // Unproject from clip space to view space using the inverse projection matrix
    vec4 viewPos = vec4(invProjection * clipPos);
    viewPos /= viewPos.w;  // Perspective divide.

    vec3 viewDir = normalize(viewPos.xyz);

    // Transform the view-space direction to world space using the inverse view matrix.
    // Use a w component of 0.0 to indicate that we're transforming a direction.
    vec4 worldDir4 = vec4(invView * vec4(viewDir, 0.0));
    vec3 direction = normalize(worldDir4.xyz);

    vec3 origin = invView[3].xyz;

    return Ray(origin, direction);
}

void main() {
    // Get the global pixel coordinate for this invocation
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(writeImage);

    Ray ray = getStartingRay(
        vec2(pixelCoord),
        vec2(imageSize),
        pushConstants.invView,
        pushConstants.invProj
    );

    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y) {
        return;
    }

    imageStore(writeImage, pixelCoord, vec4(ray.direction * 0.5 + 0.5, 1));
}