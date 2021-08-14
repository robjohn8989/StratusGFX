#version 410 core

uniform sampler2D diffuseTexture;
uniform sampler2D normalMap;
uniform sampler2D depthMap;
uniform sampler2D roughnessMap;
uniform sampler2D ambientOcclusionMap;
uniform sampler2D metalnessMap;

uniform bool textured = false;
uniform bool normalMapped = false;
uniform bool depthMapped = false;
uniform bool roughnessMapped = false;
uniform bool ambientMapped = false;
uniform bool metalnessMapped = false;

//uniform float fsShininessVals[MAX_INSTANCES];
//uniform float fsShininess = 0.0;
in float fsRoughness;
uniform float heightScale = 0.1;

/**
 * Information about the camera
 */
uniform vec3 viewPosition;

/**
 * Fragment information. All values should be
 * in world space.
 */
in vec3 fsPosition;
in vec3 fsNormal;
in vec2 fsTexCoords;
in mat4 fsModel;
in vec3 fsDiffuseColor;
in vec3 fsBaseReflectivity; // Ex: vec3(0.03-0.04) for plastics
in float fsMetallic; // Between 0 and 1 where 0 is not metallic at all and 1 is purely metallic
//in float fsfsShininess;

/**
 * Tangent space -> world space
 */
in mat3 fsTbnMatrix;

// GBuffer outputs
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gBaseReflectivity;
layout (location = 4) out vec3 gRoughnessMetallicAmbient;

// See https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
vec2 calculateDepthCoords(vec2 texCoords, vec3 viewDir) {
    float height = texture(depthMap, texCoords).r;
    vec2 p = viewDir.xy * (height * 0.005);
    return texCoords - p;
}

void main() {
    vec3 viewDir = normalize(viewPosition - fsPosition);
    vec2 texCoords = fsTexCoords;
    if (depthMapped) {
        texCoords = calculateDepthCoords(texCoords, viewDir);
        if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0) {
            discard;
        }
    }

    vec3 baseColor = fsDiffuseColor;
    vec3 normal = fsNormal;
    float roughness = fsRoughness;
    float ao = 1.0;
    float metallic = fsMetallic;

    if (textured) {
        baseColor = texture(diffuseTexture, texCoords).rgb;
    }

    if (normalMapped) {
        normal = texture(normalMap, texCoords).rgb;
        // Normals generally have values from [-1, 1], but inside
        // an OpenGL texture they are transformed to [0, 1]. To convert
        // them back, we multiply by 2 and subtract 1.
        normal = normal * 2.0 - 1.0;
        normal = normalize(fsTbnMatrix * normal);
    }

    if (roughnessMapped) {
        roughness = texture(roughnessMap, texCoords).r;
    }

    if (ambientMapped) {
        ao = texture(ambientOcclusionMap, texCoords).r;
    }

    if (metalnessMapped) {
        metallic = texture(metalnessMap, texCoords).r;
    }

    // Coordinate space is set to world
    gPosition = fsPosition;
    gNormal = normal;
    gAlbedo = baseColor;
    gBaseReflectivity = fsBaseReflectivity;
    gRoughnessMetallicAmbient = vec3(roughness, metallic, ao);
}