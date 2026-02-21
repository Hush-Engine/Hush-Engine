// basic_triangle.slang

struct Uniforms
{
    float4x4 mvp;
    float4 tintColor;
    float time;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Uniforms> uniforms;

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};

static const float2 kPositions[3] = {
    float2(0.0, 0.5),   // top
    float2(-0.5, -0.5), // bottom-left
    float2(0.5, -0.5),  // bottom-right
};

static const float3 kColors[3] = {
    float3(1.0, 0.0, 0.0), // red
    float3(0.0, 1.0, 0.0), // green
    float3(0.0, 0.0, 1.0), // blue
};

static const float2 kUVs[3] = {
    float2(0.5, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 1.0),
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexID: SV_VertexID)
{
    VertexOutput output;

    float2 pos = kPositions[vertexID];

    // Apply MVP transform
    output.position = mul(uniforms.mvp, float4(pos, 0.0, 1.0));
    output.color = kColors[vertexID];
    output.uv = kUVs[vertexID];

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Base vertex color
    float3 color = input.color;

    // Apply tint from uniforms
    color *= uniforms.tintColor.rgb;

    // Subtle time-based pulse on alpha for demonstration
    float alpha = uniforms.tintColor.a * (0.8 + 0.2 * sin(uniforms.time * 2.0));

    return float4(color, alpha);
}
