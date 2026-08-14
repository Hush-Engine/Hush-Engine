struct Uniforms
{
    float4x4 mvp;
    float4   tintColor;
    float    time;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Uniforms> uniforms;

[[vk::binding(1, 0)]]
Texture2D<float4> tex;

[[vk::binding(2, 0)]]
SamplerState texSampler;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

static const float2 kPositions[6] = {
    float2(-0.5,  0.5),
    float2(-0.5, -0.5),
    float2( 0.5, -0.5),
    float2(-0.5,  0.5),
    float2( 0.5, -0.5),
    float2( 0.5,  0.5),
};

static const float2 kUVs[6] = {
    float2(0.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 1.0),
    float2(0.0, 0.0),
    float2(1.0, 1.0),
    float2(1.0, 0.0),
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.position = float4(kPositions[vertexID], 0.0, 1.0);
    output.uv       = kUVs[vertexID];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return tex.Sample(texSampler, input.uv);
}
