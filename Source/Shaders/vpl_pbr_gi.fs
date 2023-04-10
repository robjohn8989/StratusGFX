STRATUS_GLSL_VERSION

#extension GL_ARB_bindless_texture : require

#include "pbr.glsl"
#include "pbr2.glsl"
#include "vpl_common.glsl"

// Input from vertex shader
in vec2 fsTexCoords;
out vec3 color;

#define MAX_SAMPLES_PER_PIXEL 20
#define MAX_SHADOW_SAMPLES_PER_PIXEL 25

// GBuffer information
uniform sampler2D gDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gBaseReflectivity;
uniform sampler2D gRoughnessMetallicAmbient;

// Screen space ambient occlusion
uniform sampler2DRect ssao;

// Camera information
uniform vec3 viewPosition;

// Shadow information
//uniform samplerCube shadowCubeMaps[MAX_TOTAL_VPLS_PER_FRAME];
uniform vec3 infiniteLightColor;

// Window information
uniform int viewportWidth;
uniform int viewportHeight;

// in/out frame texture
uniform sampler2D screen;

// Screen tile information
uniform int numTilesX;
uniform int numTilesY;

uniform mat4 invProjectionView;

// Used for random number generation
uniform float time;

layout (std430, binding = 0) readonly buffer inputBlock1 {
    VplData lightData[];
};

layout (std430, binding = 1) readonly buffer inputBlock2 {
    int numVisible;
};

layout (std430, binding = 2) readonly buffer inputBlock3 {
    int vplVisibleIndex[];
};

uniform samplerCubeArray shadowCubeMaps;

layout (std430, binding = 3) readonly buffer inputBlock4 {
    int shadowIndices[];
};

vec3 performLightingCalculations(vec3 screenColor, vec2 pixelCoords, vec2 texCoords) {
    if (length(screenColor) > 0.5) discard;

    // ivec2 numTiles = ivec2(numTilesX, numTilesY);
    // // For example: 16, 9
    // ivec2 multiplier = ivec2(viewportWidth, viewportHeight) / numTiles;
    // uvec2 tileCoords = uvec2(pixelCoords / vec2(multiplier));
    // //uvec2 tileCoords = uvec2(pixelCoords);
    // if (tileCoords.x >= numTiles.x || tileCoords.y >= numTiles.y) return screenColor;

    // // Each entry in the activeLightIndicesPerTile buffer has MAX_VPLS_PER_TILE entries
    // int baseTileIndex = int(tileCoords.x + tileCoords.y * numTiles.x);
    // // numActiveVPLsPerTile only has one int per entry
    // int numActiveVPLs = tileData[baseTileIndex].numVisible;
    // if (numActiveVPLs > MAX_VPLS_PER_TILE) return screenColor;

    float depth = textureLod(gDepth, texCoords, 0).r;
    vec3 fragPos = worldPositionFromDepth(texCoords, depth, invProjectionView);
    //vec3 fragPos = texture(gPosition, texCoords).rgb;
    vec3 viewDir = normalize(viewPosition - fragPos);
    float distToCamera = length(viewPosition - fragPos);

    vec3 baseColor = textureLod(gAlbedo, texCoords, 0).rgb;
    vec3 normal = normalize(textureLod(gNormal, texCoords, 0).rgb * 2.0 - vec3(1.0));
    float roughness = textureLod(gRoughnessMetallicAmbient, texCoords, 0).r;
    float metallic = textureLod(gRoughnessMetallicAmbient, texCoords, 0).g;
    // Note that we take the AO that may have been packed into a texture and augment it by SSAO
    // Note that singe SSAO is sampler2DRect, we need to sample in pixel coordinates and not texel coordinates
    float ambientOcclusion = clamp(texture(ssao, pixelCoords).r, 0.35, 1.0);
    float ambient = textureLod(gRoughnessMetallicAmbient, texCoords, 0).b * ambientOcclusion;
    vec3 baseReflectivity = textureLod(gBaseReflectivity, texCoords, 0).rgb;

    vec3 vplColor = vec3(0.0); //screenColor;

    int maxLights = numVisible < MAX_SAMPLES_PER_PIXEL ? numVisible : MAX_SAMPLES_PER_PIXEL;
    int maxShadowLights = numVisible < MAX_SHADOW_SAMPLES_PER_PIXEL ? numVisible : MAX_SHADOW_SAMPLES_PER_PIXEL;

    // Used to seed the pseudo-random number generator
    // See https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
    vec3 seed = vec3(0.0, 0.0, time);
    float shadowFactor = 0.0;
    for (int baseLightIndex = 0; baseLightIndex < maxShadowLights; ++baseLightIndex) {
        // seed.z += 1.0;
        // float rand = random(seed);
        // int lightIndex = vplVisibleIndex[int(numVisible * rand)];
        int lightIndex = vplVisibleIndex[baseLightIndex];
        vec3 lightPosition = lightData[lightIndex].position.xyz;

        shadowFactor += calculateShadowValue1Sample(shadowCubeMaps, shadowIndices[lightIndex], lightData[lightIndex].farPlane, fragPos, lightPosition, dot(lightPosition - fragPos, normal));
    }

    shadowFactor /= float(maxShadowLights);

    seed = vec3(gl_FragCoord.xy, time);
    for (int baseLightIndex = 0 ; baseLightIndex < maxLights; baseLightIndex += 1) {
        seed.z += 1000.0;
        float rand = random(seed);
        // Calculate true light index via lookup into active light table
        //int lightIndex = tileData[baseTileIndex].indices[baseLightIndex];
        //int lightIndex = vplVisibleIndex[baseLightIndex];
        int lightIndex = vplVisibleIndex[int(numVisible * rand)];
        //if (lightIndex > MAX_TOTAL_VPLS_PER_FRAME) continue;

        vec3 lightPosition = lightData[lightIndex].position.xyz;
        float lightRadius = lightData[lightIndex].radius;
        vec3 lightColor = lightData[lightIndex].color.xyz;
        float lightIntensity = length(lightColor);
        //float distance = length(lightPosition - fragPos);
        //float ratio = distance / lightRadius;
        //if (distance > lightRadii[lightIndex]) continue;

        // float shadowFactor = 0.0;
        // if (distToCamera < 500) {
        //     shadowFactor = calculateShadowValue1Sample(shadowCubeMaps[lightIndex], lightData[lightIndex].farPlane, fragPos, lightPosition, dot(lightPosition - fragPos, normal));
        // }
        // Depending on how visible this VPL is to the infinite light, we want to constrain how bright it's allowed to be
        //shadowFactor = lerp(shadowFactor, 0.0, vpl.shadowFactor);

        vplColor = vplColor + ambientOcclusion * calculateVirtualPointLighting2(fragPos, baseColor, normal, viewDir, lightPosition, lightColor, distToCamera, lightRadius, roughness, metallic, ambient, shadowFactor, baseReflectivity);
    }

    return boundHDR(vplColor);
}

void main() {
    color = performLightingCalculations(textureLod(screen, fsTexCoords, 0).rgb, gl_FragCoord.xy, fsTexCoords);
    // See https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
    // vec3 point = vec3(gl_FragCoord.xy, time);
    // point = vec3(random(point));

    // color = point * 1.0;
}