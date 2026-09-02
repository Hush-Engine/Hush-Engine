//=== mesh.slang ============================================================
//  Port of mesh.vert / mesh.frag + includes to Slang for WebGPU via Slang
//  Original GLSL includes inlined below:
//    input_structures.glsl → bind group layouts
//    pbrUtils.glsl         → PBR BRDF math
//    shaderMath.glsl       → utility functions
//    lighting.glsl         → directional light PBR
//===========================================================================

typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;

//  input_structures.glsl
struct SceneData
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewproj;
    float4 ambientColor;
    float4 sunlightDirection; // .w = sun power
    float4 sunlightColor;
};

struct ModelData
{
    float4x4 modelMatrix;
};

struct PBRMaterialData
{
    float4 colorFactors;
    float4 metal_rough_factors;
    float4 emissionFactors;
    float alphaCutoff;
    uint32_t optionFlags;
};

// Set 0 — per-frame / per-draw
[[vk::binding(0, 0)]]
ConstantBuffer<SceneData> u_sceneData;
[[vk::binding(1, 0)]]
ConstantBuffer<ModelData> modelData;

// Set 1 — per-material
[[vk::binding(0, 1)]]
ConstantBuffer<PBRMaterialData> materialData;
[[vk::binding(1, 1)]]
Texture2D<float4> colorTex;
[[vk::binding(2, 1)]]
Texture2D<float4> metalRoughTex;
[[vk::binding(3, 1)]]
Texture2D<float4> normalTex;
[[vk::binding(4, 1)]]
Texture2D<float4> emissiveTex;
[[vk::binding(5, 1)]]
SamplerState defaultSampler;

static const float PI = 3.14159265;
static const float EPSILON = 0.00001f;

float3 FresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 FresnelSchlickRoughness(float3 F0, float cosTheta, float roughness)
{
    return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float GeometrySchlickGGX(float NdotV, float alpha)
{
    float k = alpha / 2.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float alpha)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, alpha);
    float ggx1 = GeometrySchlickGGX(NdotL, alpha);
    return ggx1 * ggx2;
}

float GaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

float GaSchlickGGX(float cosLi, float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return GaSchlickG1(cosLi, k) * GaSchlickG1(NdotV, k);
}

float DistributionGGX(float3 N, float3 H, float alpha)
{
    float a2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

static const float Epsilon = 0.00001;

float3 calculateTangentGramSchmidt(float3 normal, float3 tangent)
{
    return tangent - dot(tangent, normal) * normal;
}

float3 viewMatExtractFwd(float4x4 viewMatrix)
{
    return float3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
}

float3 viewMatExtractPos(float4x4 viewMatrix)
{
    return -float3(viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]);
}

float3 scalarPow(float3 v, float n)
{
    return float3(pow(v.x, n), pow(v.y, n), pow(v.z, n));
}

float4 ToLinear(float4 sRGB)
{
    bool4 cutoff = sRGB < float4(0.04045);
    float4 higher = pow((sRGB + float4(0.055)) / float4(1.055), float4(2.4));
    float4 lower = sRGB / float4(12.92);
    return lerp(higher, lower, (float4)cutoff);
}

bool HasCompositeFlag(int base, int compositeFlag)
{
    return (base & compositeFlag) != 0;
}

// Lighting
struct PBRParameters
{
    float3 Albedo;
    float Roughness;
    float Metalness;
    float3 Normal;
    float3 View;
    float NdotV;
};

float3 CalculateDirLights(PBRParameters params)
{
    const float3 Fdielectric = float3(0.04);
    float3 F0 = lerp(Fdielectric, params.Albedo, params.Metalness);

    float3 result = float3(0.0);
    for (int i = 0; i < 1; i++)
    {
        float sunPower = u_sceneData.sunlightDirection.w;
        if (sunPower == 0.0)
            continue;

        float3 Li = u_sceneData.sunlightDirection.xyz;
        float3 Lradiance = u_sceneData.sunlightColor.xyz * sunPower;
        float3 Lh = normalize(Li + params.View);

        float cosLi = max(0.0, dot(params.Normal, Li));

        float3 F = FresnelSchlick(F0, max(0.0, dot(Lh, params.View)));
        float D = DistributionGGX(params.Normal, Lh, params.Roughness);
        float G = GaSchlickGGX(cosLi, params.NdotV, params.Roughness);

        float3 kd = (1.0 - F) * (1.0 - params.Metalness);
        float3 diffuseBRDF = kd * params.Albedo;

        float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * params.NdotV);
        specularBRDF = clamp(specularBRDF, float3(0.0), float3(10.0));

        result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
    }
    return result;
}

static const int USE_NORMALS_FLAG = 0x1;
static const int DEBUG_NORMALS_FLAG = 0x2;

struct VertexInput
{
    [[vk::location(0)]]
    float3 position : POSITION;
    [[vk::location(1)]]
    float3 normal : NORMAL;
    [[vk::location(2)]]
    float4 color : COLOR;
    [[vk::location(3)]]
    float4 tangent : TANGENT;
    [[vk::location(4)]]
    float2 uv : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_Position;
    [[vk::location(0)]]
    float3 normal;
    [[vk::location(1)]]
    float3 outColor;
    [[vk::location(2)]]
    float2 outUV;
    [[vk::location(3)]]
    float3 outTangent;
    [[vk::location(4)]]
    float outHandedness;
    [[vk::location(5)]]
    float3 outWorldPos;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    float4 position = float4(input.position, 1.0);

    VertexOutput output;
    output.position = mul(mul(u_sceneData.viewproj, modelData.modelMatrix), position);
    output.outWorldPos = mul(modelData.modelMatrix, position).xyz;

    float3x3 modelMat3 = (float3x3)modelData.modelMatrix;
    output.normal = normalize(mul(modelMat3, input.normal));
    output.outTangent = normalize(mul(modelMat3, input.tangent.xyz));
    output.outHandedness = input.tangent.w;
    output.outColor = input.color.xyz * materialData.colorFactors.xyz;
    output.outUV = input.uv;

    return output;
}

struct FragmentOutput
{
    [[vk::location(0)]]
    float4 color;
};

[shader("fragment")]
FragmentOutput fragMain(VertexOutput input)
{
    float4 texColor = colorTex.Sample(defaultSampler, input.outUV);
    float alpha = texColor.w * materialData.colorFactors.a;
    if (alpha < materialData.alphaCutoff)
    {
        discard;
    }

    PBRParameters params;
    params.Albedo = texColor.rgb * scalarPow(input.outColor, 2.2);

    float4 metalRough = metalRoughTex.Sample(defaultSampler, input.outUV);
    params.Metalness = metalRough.b * materialData.metal_rough_factors.x;
    params.Roughness = metalRough.g * materialData.metal_rough_factors.y;
    params.Roughness = max(params.Roughness, 0.05);

    // Calculate normal
    if (HasCompositeFlag(materialData.optionFlags, USE_NORMALS_FLAG))
    {
        float3 tangent = calculateTangentGramSchmidt(input.normal, input.outTangent);
        float3 biTangent = cross(input.normal, tangent) * input.outHandedness;
        float3x3 TBN = float3x3(tangent, biTangent, input.normal);
        float3 localNormal = 2.0 * normalTex.Sample(defaultSampler, input.outUV).rgb - 1.0;
        params.Normal = normalize(mul(TBN, localNormal));
    }
    else
    {
        params.Normal = normalize(input.normal);
    }

    if (HasCompositeFlag(materialData.optionFlags, DEBUG_NORMALS_FLAG))
    {
        FragmentOutput o;
        o.color = float4(params.Normal, alpha);
        return o;
    }

    float3 fragToCamDir = normalize(viewMatExtractPos(u_sceneData.view) - input.outWorldPos);
    params.View = fragToCamDir;
    params.NdotV = max(dot(params.Normal, params.View), 0.0);

    float4 texEmission = emissiveTex.Sample(defaultSampler, input.outUV);
    float3 emission =
        (scalarPow(texEmission.xyz, 2.2) * materialData.emissionFactors.xyz) * materialData.emissionFactors.w;

    float3 directLight = CalculateDirLights(params);
    float3 ambient = u_sceneData.ambientColor.rgb * texColor.rgb * 0.5;

    FragmentOutput o;
    vec3 finalColor = vec3(ambient + directLight + emission);
    finalColor = finalColor / (finalColor + vec3(1.0f));
    const float gamma = 1.0f;
    finalColor = pow(finalColor, vec3(1.0f / gamma, 1.0f / gamma, 1.0f / gamma));
    o.color = vec4(finalColor, alpha);
    return o;
}
