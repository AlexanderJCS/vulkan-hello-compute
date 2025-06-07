#ifndef RAYMARCH_TONEMAPPING_H
#define RAYMARCH_TONEMAPPING_H

#ifdef __cplusplus

#include <glm/glm.hpp>
using glm::mat4;

#endif

struct ComputePushConsts {
    mat4 invView;
    mat4 invProj;
};

#endif  // RAYMARCH_TONEMAPPING_H
