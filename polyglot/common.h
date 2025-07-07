#ifndef RAYMARCH_TONEMAPPING_H
#define RAYMARCH_TONEMAPPING_H

#ifdef __cplusplus

#include <glm/glm.hpp>
using glm::mat4;
using glm::vec2;
#endif

struct Agent {
    vec2 position;
    float angle;
};

struct ComputePushConsts {
    mat4 invView;
    mat4 invProj;
    float deltaTime;
};

#endif  // RAYMARCH_TONEMAPPING_H
