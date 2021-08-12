#version 410 core

layout (location = 0)  in vec3 position;
layout (location = 1)  in vec2 texCoords;
layout (location = 2)  in vec3 normal;
layout (location = 3)  in vec3 tangent;
layout (location = 4)  in vec3 bitangent;
layout (location = 9)  in vec3 baseReflectivity;
layout (location = 10) in float metallic;
layout (location = 11) in float roughness;
layout (location = 12) in mat4 model;

uniform mat4 projection;
uniform mat4 view;

/**
 * Information about the camera
 */
uniform vec3 viewPosition;

smooth out vec3 fsPosition;
out vec3 fsNormal;
smooth out vec2 fsTexCoords;

// Made using the tangent, bitangent and normal
out mat3 fsTbnMatrix;
out vec3 fsTanViewPosition;
out vec3 fsTanFragPosition;
out float fsRoughness;
out mat4 fsModel;
out vec3 fsBaseReflectivity;
out float fsMetallic;

void main() {
    //mat4 model = modelMats[gl_InstanceID];
    vec4 pos = model * vec4(position, 1.0);
    fsPosition = pos.xyz;
    fsTexCoords = texCoords;
    fsNormal = normalize(mat3(model) * normal);
    // @see https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    /*
    vec3 t = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 b = normalize(vec3(model * vec4(bitangent, 0.0)));
    vec3 n = normalize(vec3(model * vec4(normal, 0.0)));
    */
    mat3 normalMatrix = mat3(model); //transpose(inverse(mat3(model)));
    vec3 t = normalize(normalMatrix * tangent);
    //b = normalize(vec3(model * vec4(b, 0.0)));
    vec3 b = normalize(normalMatrix * bitangent);
    vec3 n = normalize(normalMatrix * normal);
    // Gram-Schmidt
    //t = normalize(t - dot(t, n) * n);
    //vec3 b = cross(n, t);

    //t = normalize(t - dot(t, n) * n);
    //vec3 b = cross(n, t);
    fsTbnMatrix = transpose(mat3(t, b, n));
    fsTanViewPosition = fsTbnMatrix * viewPosition;
    fsTanFragPosition = fsTbnMatrix * fsPosition;
    //fsShininess = shininessVals[gl_InstanceID];
    fsRoughness = roughness;
    fsModel = model;
    fsBaseReflectivity = baseReflectivity;
    fsMetallic = metallic;
    gl_Position = projection * view * pos;
}