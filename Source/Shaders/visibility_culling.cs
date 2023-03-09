STRATUS_GLSL_VERSION

#extension GL_ARB_bindless_texture : require

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

#include "common.glsl"
#include "aabb.glsl"

layout (std430, binding = 2) readonly buffer inputBlock2 {
    mat4 modelTransforms[];
};

layout (std430, binding = 4) readonly buffer inputBlock3 {
    mat4 globalTransforms[];
};

layout (std430, binding = 3) readonly buffer inputBlock4 {
    AABB aabbs[];
};

layout (std430, binding = 1) writeonly buffer outputBlock1 {
    DrawElementsIndirectCommand drawCalls[];
};

uniform uint numDrawCalls;

void main() {
    // Defines local work group from layout local size tag above
    uint localWorkGroupSize = gl_WorkGroupSize.x * gl_WorkGroupSize.y;
   
    for (uint i = gl_LocalInvocationIndex; i < numDrawCalls; i += localWorkGroupSize) {
        AABB aabb = transformAabb(aabbs[i], globalTransforms[i]);
        if (!isAabbVisible(aabb)) {
            drawCalls[i].instanceCount = 0;
        }
        else {
            drawCalls[i].instanceCount = 1;
        }
    }
}