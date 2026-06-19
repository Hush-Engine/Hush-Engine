// Shader implementation based off of https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8

struct FragmentOutput {
	[vk_location(0)] float4 color;
};

typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;

static const float4 xAxisColor = float4(1.0, 0.0, 0.0, 1.0);
static const float4 zAxisColor = float4(0.0, 0.0, 1.0, 1.0);


struct ViewUniforms {
    float4x4 view;
    vec4 pos;
    float2 resolution;
    float farPlane;
};

struct VertOutput {
    float4 clipPos : SV_Position;
    [vk_location(0)] float2 ndc;
    [vk_location(2)] float2  uv;
    [vk_location(4)] float  far;
}


static const float MAJOR_THICK = 0.4;
static const float AXIS_THICK = 0.5;
static const float MINOR_THICK = 0.05;
static const float MAJOR_GRID_DIV = 10.0;
static const float HEIGHT = -1;
static const float FOV = 90;
static const float PI = 3.14159;
static const float2 RESOLUTION = float2(480, 600);
static const float3 COLOR = float3(0.5, 0.5, 0.5);
static const vec4 X_AXIS_COLOR = vec4(1.0, 0.1, 0.1, 1.0);
static const vec4 Z_AXIS_COLOR = vec4(0.1, 0.1, 1.0, 1.0);

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

float GetActualDrawWidth(float intendedThickness) {
    
}

float3 GridMask(vec2 uv, vec4 uvDDXY, vec2 uvDeriv, float lineThickness) {
    bool2 invertLine = lineThickness > 0.5;
    float2 targetWidth = select(invertLine, 1.0 - lineThickness, lineThickness);
    float2 drawWidth = clamp(targetWidth, uvDeriv, 0.5);
    // float2 drawWidth = targetWidth;
    float2 lineAA = uvDeriv * 1.5;
    float2 gridUV = abs(frac(uv) * 2.0 - 1.0);
    gridUV = invertLine ? gridUV : 1.0 - gridUV;
    float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(targetWidth / drawWidth);
    grid2 = lerp(grid2, targetWidth, saturate(uvDeriv * 2.0 - 1.0));
    grid2 = invertLine ? 1.0 - grid2 : grid2;
    return float3(lerp(grid2.x, 1.0, grid2.y), targetWidth / drawWidth);
    
}

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

    float3 camPos = view.pos.xyz;
    float3 camRight = normalize(vec3(view.view[0][0], view.view[1][0], view.view[2][0]));
    float3 camUp = normalize(vec3(view.view[0][1], view.view[1][1], view.view[2][1]));
    float3 camForward = normalize(-vec3(view.view[0][2], view.view[1][2], view.view[2][2]));

    // HARDCODED: 90 degree vertical FOV => tan(45°) = 1
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
    if (t <= 0.01)
    {
        FragmentOutput o;
        o.color = float4(0.0, 0.0, 0.0, 0.0);
        return o;
    }

    float3 hit = camPos + rayDir * t;

    float2 uv = hit.xz;

    float4 uvDDXYMinor = float4(ddx(uv), ddy(uv)); //
    float2 uvDerivMinor = float2(length(uvDDXYMinor.xz), length(uvDDXYMinor.yw)); //
    vec3 minorGridRes = GridMask(uv, uvDDXYMinor, uvDerivMinor, MINOR_THICK);
    float minorGrid = minorGridRes.x;

    float2 uvMajor = uv / MAJOR_GRID_DIV;
    float4 uvDDXYMajor = float4(ddx(uvMajor), ddy(uvMajor)); //
    float2 uvDerivMajor = float2(length(uvDDXYMajor.xz), length(uvDDXYMajor.yw)); //
    vec3 gridRes = GridMask(uvMajor, uvDDXYMajor, uvDerivMajor, MAJOR_THICK / MAJOR_GRID_DIV);
    float2 maxDrawWidth = gridRes.yz;
    float grid = gridRes.x;

    float2 uvSuper = uv / (MAJOR_GRID_DIV * 10);
    float4 uvDDXYSuper = float4(ddx(uvSuper), ddy(uvSuper)); //
    float2 uvDerivSuper = float2(length(uvDDXYSuper.xz), length(uvDDXYSuper.yw)); //
    float superGrid1 = GridMask(uvSuper, uvDDXYSuper, uvDerivSuper, MAJOR_THICK / (MAJOR_GRID_DIV * 10)).x;


    float2 uvSuper2 = uv / (MAJOR_GRID_DIV * 100);
    float4 uvDDXYSuper2 = float4(ddx(uvSuper2), ddy(uvSuper2)); //
    float2 uvDerivSuper2 = float2(length(uvDDXYSuper2.xz), length(uvDDXYSuper2.yw)); //
    float superGrid2 = GridMask(uvSuper2, uvDDXYSuper2, uvDerivSuper2, MAJOR_THICK / (MAJOR_GRID_DIV * 100)).x;

    float axisLineWidth = max(AXIS_THICK, MAJOR_THICK);

    float2 axisDrawWidth = max(axisLineWidth, uvDerivMinor);
    float2 axisLineAA = uvDerivMinor * 1.5;

    float2 axisLines2 = smoothstep(
        axisDrawWidth + axisLineAA,
        axisDrawWidth - axisLineAA,
        abs(uv * 2.0)
    );

    axisLines2 *= saturate(axisDrawWidth);
    float xAxis = axisLines2.y; // uv.y == hit.z == 0  →  X axis
    float zAxis = axisLines2.x; // uv.x == hit.x == 0  →  Z axis    

    float camHeight = abs(camPos.y);

    o.color = vec4(0);
    o.color = lerp(o.color, vec4(COLOR, 1.0), minorGrid);
    o.color = lerp(o.color, vec4(COLOR, 1.0), grid);
    o.color = lerp(o.color, vec4(COLOR, 1.0), superGrid1);
    o.color = lerp(o.color, vec4(COLOR, 1.0), superGrid2);
    o.color = lerp(o.color, X_AXIS_COLOR, xAxis);
    o.color = lerp(o.color, Z_AXIS_COLOR, zAxis);

    return o;
}
