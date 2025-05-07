#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"
#include "pbrUtils.glsl"
#include "shaderMath.glsl"
#include "lighting.glsl"

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in float inHandedness;
layout(location = 5) in vec3 inWorldPos;

layout(location = 0) out vec4 outFragColor;


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
    m_params.Albedo = scalarPow(texColor.rgb * inColor, 2.2);
    vec4 metalRough = texture(metalRoughTex, inUV);
    m_params.Metalness = metalRough.b * materialData.metal_rough_factors.x;
    m_params.Roughness = metalRough.g * materialData.metal_rough_factors.y;
    m_params.Roughness = max(m_params.Roughness, 0.05);
    // Calculate normal related stuff
    vec3 tangent = calculateTangentGramSchmidt(inNormal, inTangent);
    vec3 biTangent = cross(inNormal, tangent) * inHandedness;
    TBN = mat3(tangent, biTangent, inNormal);

    vec3 localNormal = 2.0 * texture(normalTex, inUV).rgb - 1.0;
    m_params.Normal = normalize(TBN * localNormal);

    // Calculate the light once we're done with normal calculations
    vec3 viewDirection = viewMatExtractFwd(u_sceneData.view);

    // TODO: replace with IBL for point lights
    vec3 fragToCamDir = normalize(viewMatExtractPos(u_sceneData.view) - inWorldPos);
    m_params.View = fragToCamDir;

	m_params.NdotV = max(dot(m_params.Normal, m_params.View), 0.0);

    vec4 texEmission = texture(emissiveTex, inUV);
    vec3 emission = (scalarPow(texEmission.xyz, 2.2) * materialData.emissionFactors.xyz) * materialData.emissionFactors.w;
    
    vec3 directLight = CalculateDirLights();

    vec3 ambient = u_sceneData.ambientColor.rgb * texColor.rgb * 0.1;
    outFragColor = vec4(ambient + directLight + emission, texColor.a);
}
