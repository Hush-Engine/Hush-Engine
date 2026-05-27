struct FragmentOutput {
	[vk_location(0)] float4 color;
};

typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;

static const float4 xAxisColor = float4(1.0, 0.0, 0.0, 1.0);
static const float4 zAxisColor = float4(0.0, 0.0, 1.0, 1.0);


// Vertex structs
struct ViewUniforms {
    float4x4 invviewproj;
    vec4 pos;
	vec4 forward;
	vec4 up;
	vec4 right;
    float2 resolution;
    float farPlane;
};


struct VertOutput {
    float4 clipPos : SV_Position;
    [vk_location(0)] float2 ndc;
    [vk_location(2)] float2  uv;
    [vk_location(4)] float  far;
}


static const float THICKNESS = 0.1;
static const float HEIGHT = -1;
static const float FOV = 90;
static const float PI = 3.14159;
static const float2 RESOLUTION = float2(480, 600);
static const float3 COLOR = float3(1.0, 1.0, 1.0);


ConstantBuffer<ViewUniforms> view;

static const float2 verts[6] = {
    float2(-1.0, -1.0),
    float2( 1.0, -1.0),
    float2( 1.0,  1.0),

    float2(-1.0, -1.0),
    float2( 1.0,  1.0),
    float2(-1.0,  1.0),
};

static const float2 uvs[6] = {
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0),

    float2(0.0, 0.0),
    float2(1.0, 1.0),
    float2(0.0, 1.0),
};

[shader("vertex")]
VertOutput vertMain(uint vertexID : SV_VertexID)
{
    VertOutput o;
    o.clipPos = float4(verts[vertexID], 0.0, 1.0);
    o.uv = uvs[vertexID];
    return o;
}

[shader("fragment")]
FragmentOutput fragmentMain(VertOutput input)
{
    FragmentOutput o;
    // NDC from interpolated UV
    float2 ndc = float2(
        input.uv.x * 2.0 - 1.0,
        input.uv.y * 2.0 - 1.0
    );    
    // Hardcoded camera basis for intended coordinate system:
    float3 camPos     = view.pos.xyz;
    // float3 camRight   = normalize(view.right);
    float3 camRight   = normalize(view.right.xyz);
    float3 camUp      = normalize(view.up.xyz);
    float3 camForward = normalize(view.forward.xyz);
    // float3 camForward = normalize(float3(0.0, -0.25, view.forward.z));

    // 90 degree vertical FOV => tan(45°) = 1
    float aspect = view.resolution.x / view.resolution.y;
    float tanHalfFov = 1.0;

    float3 rayDir = normalize(
        camForward +
        ndc.x * aspect * tanHalfFov * camRight +
        ndc.y * tanHalfFov * camUp
    );

    // Intersect ground plane Y = 0
    float denom = rayDir.y;
    if (abs(denom) < 1e-6)
    {
        FragmentOutput o;
        o.color = float4(0.0, 0.0, 0.0, 0.0);
        return o;
    }

    float t = -camPos.y / denom;
    if (t < 0.0)
    {
        FragmentOutput o;
        o.color = float4(0.0, 0.0, 0.0, 0.0);
        return o;
    }

    float3 hit = camPos + rayDir * t;

    // Solid world-space debug color

    float2 uv = hit.xz;

    float2 uvDeriv = fwidth(uv); //
    float2 lineAA = uvDeriv * 1.5; //
    float2 gridUV = 1.0 - abs(frac(uv) * 2.0 - 1.0);
    float2 grid2 = smoothstep(THICKNESS + lineAA, THICKNESS - lineAA, gridUV);
    float grid = lerp(grid2.x, 1.0, grid2.y);

    // o.color = float4(
    //     frac(hit.x * 0.1),
    //     frac(hit.z * 0.1),
    //     0.0,
    //     1.0
    // );

    o.color = float4(COLOR, grid);

    return o;
}
