#version 460

#include "common.h"

layout (push_constant) uniform PushConsts {
    ComputePushConsts pushConstants;
};

layout (local_size_x = 32, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba8) writeonly uniform image2D outImage;

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

float sdSphere(vec3 p, float s) {
    return length(p) - s;
}

float mdbDE(vec3 c) {
    vec3 z = c;
    float dr = 1.0;
    float r = length(z);
    for (int i = 0; i < 6 && r < 2.0; i++) {
        float powr = pow(r, 7.0);
        dr = dr * powr * 8.0 + 1.0;
        float theta = acos(z.z / r) * 8.0;
        float phi = atan(z.y, z.x) * 8.0;
        z = r * powr * vec3(sin(theta) * vec2(cos(phi), sin(phi)), cos(theta)) + c;
        r = length(z);
    }
    return min(0.5 * log(r) * r / dr, 1.0);
}

float DE(vec3 p) {
    float s = 3.0f, e;
    float trap = 1000.0;
    for ( int i = 0; i++ < 8; ) {
        p = mod( p - 1.0f, 2.0f ) - 1.0f;
        s *= e = 1.4f / dot( p, p );
        p *= e;
        trap = min(trap, mdbDE(p - vec3(2.88, -0.93, -0.93)) / s);
    }
    return trap;
}

float sdfWorld(in vec3 p) {
//    float sphere = sdSphere(p, 1);
//
//    return DE(p);
    return abs(p.y);
}

vec3 calcNormal(in vec3 p) {
    const vec3 delta = vec3(0.001, 0, 0);

    float gradientX = sdfWorld(p + delta.xyy) - sdfWorld(p - delta.xyy);
    float gradientY = sdfWorld(p + delta.yxy) - sdfWorld(p - delta.yxy);
    float gradientZ = sdfWorld(p + delta.yyx) - sdfWorld(p - delta.yyx);

    return normalize(vec3(gradientX, gradientY, gradientZ));
}

vec3 calcLighting(vec3 wo, vec3 p) {
    vec3 objectColor = vec3(0.4, 0.8, 0.4);

    vec3 lightPos = vec3(0, 3, 0);
    vec3 lightColor = vec3(1, 0.4, 0.4);

    vec3 n = calcNormal(p);
    vec3 wi = normalize(lightPos - p);
    vec3 h = normalize(wi + wo);

    float ambientStrength = 0.8;
    vec3 ambientLightColor = vec3(245/255.0);
    vec3 ambient = ambientStrength * ambientLightColor;

    float diffuseStrength = 0.8;
    vec3 diffuse = diffuseStrength * lightColor * max(0.0, dot(n, wi));

    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-wi, n);
    float spec = pow(max(dot(n, h), 0.0), 64);
    vec3 specular = specularStrength * lightColor * spec;

    float distSquared = dot(p - lightPos, p - lightPos);

    return (ambient + (diffuse + specular) / distSquared) * objectColor;
}

vec3 getColor(Ray ray) {
    float distTraveled = 0.0;
    const int STEPS = 3200;
    const float MIN_HIT_DIST = 0.001;
    const float MAX_TRACE_DIST = 10000;

    for (int i = 0; i < STEPS; i++) {
        vec3 pos = ray.origin + distTraveled * ray.direction;
        float d = sdfWorld(pos);

        if (d < MIN_HIT_DIST) {
            return calcLighting(-ray.direction, pos);
        }

        if (distTraveled > MAX_TRACE_DIST) {
            break;
        }

        distTraveled += d;
    }

    return vec3(245/255.0);
}

void main() {
    // Get the global pixel coordinate for this invocation
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outImage);

    Ray ray = getStartingRay(
        vec2(pixelCoord),
        vec2(imageSize),
        pushConstants.invView,
        pushConstants.invProj
    );

    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y) {
        return;
    }

    imageStore(outImage, pixelCoord, vec4(getColor(ray), 1));
}
