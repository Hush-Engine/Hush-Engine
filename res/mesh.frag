#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"
#include "pbrUtils.glsl"

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in float inHandedness;
layout(location = 5) in vec3 inWorldPos;

layout(location = 0) out vec4 outFragColor;

// Makes the normal look better for little performance cost
vec3 calculateTangentGramSchmidt(in vec3 normal, in vec3 tangent) {
    return (tangent - dot(tangent, normal) * normal);
}

vec3 viewMatExtractFwd(in mat4 viewMatrix) {
    // 8 9 and 10 idx corresponds to -fwd
    return vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
}

vec3 viewMatExtractPos(in mat4 viewMatrix) {
    // I... think this is correct, we need to negate this
    return -vec3(viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]);
}

vec3 scalarPow(in vec3 v, in float n) {
    return vec3(pow(v.x, n), pow(v.y, n), pow(v.z, n));
}

// Tangent, BiTangent and normal matrix
// Converts texture space into model space
mat3 TBN;

const float specShininess = 32.0;

void main()
{
    vec4 texColor = texture(colorTex, inUV);
    float alpha = texColor.w;
    if (alpha < materialData.alphaCutoff) {
        discard;
    }

    // PBR stuff
    // vec3 albedo = texColor.rgb * inColor;
    vec3 albedo = scalarPow(texColor.rgb * inColor, 2.2);
    vec4 metalRough = texture(metalRoughTex, inUV);
    float metallic = metalRough.b * materialData.metal_rough_factors.x;
    float roughness = metalRough.g * materialData.metal_rough_factors.y;
    // Calculate normal related stuff
    vec3 tangent = calculateTangentGramSchmidt(inNormal, inTangent);
    vec3 biTangent = cross(inNormal, tangent) * inHandedness;
    TBN = mat3(tangent, biTangent, inNormal);

    vec3 localNormal = 2.0 * texture(normalTex, inUV).rgb - 1.0;
    vec3 finalNormal = normalize(TBN * localNormal);

    // Calculate the light once we're done with normal calculations
    vec3 viewDirection = viewMatExtractFwd(sceneData.view);

    // TODO: replace with IBL for point lights
    vec3 fragToCamDir = normalize(viewMatExtractPos(sceneData.view) - inWorldPos);
    vec3 radiance = sceneData.sunlightColor.rgb * sceneData.sunlightDirection.w * PI;

    vec3 directLight = PBR(
        albedo,
        metallic,
        roughness,
        finalNormal, //N
        fragToCamDir,
        normalize(sceneData.sunlightDirection.xyz), //L
        radiance
    );

    vec3 ambient = sceneData.ambientColor.rgb * texColor.rgb * 0.1;
    outFragColor = vec4(ambient + directLight, texColor.a);
}
