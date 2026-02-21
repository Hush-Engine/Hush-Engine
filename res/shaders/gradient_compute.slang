// gradient_compute.slang

struct ComputeUniforms
{
    uint   imageWidth;
    uint   imageHeight;
    float  time;
    float  _padding;
    float4 topColor;
    float4 bottomColor;
};

[[vk::binding(0, 0)]]
ConstantBuffer<ComputeUniforms> params;

struct Pixel
{
    float4 color;
};

[[vk::binding(1, 0)]]
RWStructuredBuffer<Pixel> outputPixels;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint x = dispatchThreadID.x;
    uint y = dispatchThreadID.y;

    // Bounds check — threads outside the image dimensions should exit early
    if (x >= params.imageWidth || y >= params.imageHeight)
    {
        return;
    }

    // Compute normalized UV coordinates
    float u = float(x) / float(params.imageWidth - 1);
    float v = float(y) / float(params.imageHeight - 1);

    // Vertical gradient between top and bottom colors
    float4 gradientColor = lerp(params.topColor, params.bottomColor, v);

    // Add a subtle horizontal wave based on time
    float wave = sin(u * 6.2831853 + params.time) * 0.1 + 0.9;
    gradientColor.rgb *= wave;

    // Ensure alpha is fully opaque
    gradientColor.a = 1.0;

    // Write the result to the linear output buffer
    uint pixelIndex = y * params.imageWidth + x;
    outputPixels[pixelIndex].color = gradientColor;
}
