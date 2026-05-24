
struct FragmentInput {
	[vk_location(0)] float near;
	[vk_location(1)] float gridSize;
	[vk_location(2)] float3 worldPos;
	[vk_location(3)] float3 farPoint;
	[vk_location(4)] float4x4 fragView;
	[vk_location(8)] float4x4 fragProj;
};


struct FragmentOutput {
	[vk_location(0)] float4 color;
};



static const float gridCellSize = 1.0;
static const float minPixelsBetweenCells = 2.0;
static const float4 gridColorThin = float4(0.5, 0.5, 0.5, 1.0);
static const float4 gridColorThick = float4(0.0, 0.0, 0.0, 1.0);
static const float4 xAxisColor = float4(1.0, 0.0, 0.0, 1.0);
static const float4 zAxisColor = float4(0.0, 0.0, 1.0, 1.0);


// Vertex structs
struct ViewUniforms {
    float4x4 viewproj;
    float3 pos;
    float farPlane;
};

static const float3 quadPos[4] = {
    float3(-1.0, 0.0, -1.0),
    float3( 1.0, 0.0, -1.0),
    float3( 1.0, 0.0,  1.0),
    float3(-1.0, 0.0,  1.0),
};

static const int quadIndices[6] = { 0, 2, 1, 2, 0, 3 };


struct VertOutput {
    float4 clipPos : SV_Position;
    [vk_location(0)]  float near;
    [vk_location(1)]  float far;
    [vk_location(2)]  float3 worldPos;
};

ConstantBuffer<ViewUniforms> view;


[shader("vertex")]
VertOutput vertMain(uint vertexID : SV_VertexID) {
    VertOutput output;
    int idx = quadIndices[vertexID];

    // Offset quad vertex by camera world position and far plane scale
    float3 vertPos = quadPos[idx] * view.farPlane;
    vertPos.x += view.pos.x;
    vertPos.z += view.pos.z;


    output.worldPos = vertPos;
    output.worldPos.y = view.pos.y;

    float4 vertPos4 = float4(vertPos, 1.0);

    output.clipPos = mul(view.viewproj, vertPos4);  // replaces gl_Position
    output.far = view.farPlane;
    output.near = 0.0;
    return output;
}

float log10(float x) {
    return log(x) / log(10.0);
}

float max2(float2 v) {
    return max(v.x, v.y);
}

float2 satv(float2 x) {
    return clamp(x, float2(0.0), float2(1.0));
}


// Given the current worldPos and a chosen grid cell size (gridLod),
// calculate an alpha value for the grid line based on how close we are to a cell edge.
// dudv is a screen-space estimate (via derivatives) of world-space change.
float calculateAlphaLod(float gridLod, float2 dudv, float3 worldPos) {
    float2 moduloDiv = fmod(worldPos.xz, gridLod) / dudv;
    return max2(float2(1.0) - abs(satv(moduloDiv) * 2.0 - float2(1.0)));
}

// Compute a LOD value based on the magnitude of the derivative.
float calculateLOD(float derivativeMagnitude) {
    return max(0.0, log10(derivativeMagnitude * minPixelsBetweenCells / gridCellSize) + 1.0);
}


float4 drawAxisLines(float3 fragPos3D, float2 dudv, float farPlane) {
    // Constant desired axis thickness in screen-space (pixels)
    float desiredPixelWidth = 5.0;
    float epsilon = 1e-6;
    
    // Compute a world-space thickness from the desired pixel width.
    // For the X axis (drawn along z = 0), we use the screen-space derivative of z (dudv.y),
    // and for the Z axis (drawn along x = 0), we use the derivative of x (dudv.x).
    float thicknessX = desiredPixelWidth / max(dudv.y, epsilon);
    float thicknessZ = desiredPixelWidth / max(dudv.x, epsilon);
    
    // Clamp the thickness so it never becomes too large (here capped to 10% of a grid cell).
    float maxThickness = gridCellSize * 0.1;
    thicknessX = min(thicknessX, maxThickness);
    thicknessZ = min(thicknessZ, maxThickness);
    
    // Compute the screen-space antialiasing width using fwidth.
    // Use a minimum value to avoid artifacts when fwidth is extremely small.
    float aaWidthX = max(fwidth(fragPos3D.z), 0.001);
    float aaWidthZ = max(fwidth(fragPos3D.x), 0.001);
    
    // Compute the distance from the fragment to the X and Z axes in world space.
    float distToXAxis = abs(fragPos3D.z); // X axis is at z = 0.
    float distToZAxis = abs(fragPos3D.x); // Z axis is at x = 0.
    
    // Use smoothstep with the computed antialiasing widths.
    // The idea is that when the distance (minus the desired thickness) is less than the AA width,
    // the line is smoothly blended.
    float alphaX = 1.0 - smoothstep(0.0, aaWidthX, distToXAxis - thicknessX);
    float alphaZ = 1.0 - smoothstep(0.0, aaWidthZ, distToZAxis - thicknessZ);
    
    // Optionally fade the axes out toward the far clipping plane.
    float distFromCenter = length(fragPos3D.xz);
    float farFade = 1.0 - smoothstep(farPlane * 0.9, farPlane, distFromCenter);
    alphaX *= farFade;
    alphaZ *= farFade;
    
    // Separate branches: output the X axis (red) or Z axis (blue) based on which one is stronger.
    if (alphaX > alphaZ && alphaX > 0.0) {
        return float4(xAxisColor.rgb, alphaX);
    } else if (alphaZ > 0.0) {
        return float4(zAxisColor.rgb, alphaZ);
    }
    return float4(0.0);
}



[shader("fragment")]
FragmentOutput fragmentMain(VertOutput input) {
    // Compute screen-space derivatives for the x and z worldPos components.
    float2 dVx = float2(ddx(input.worldPos.x), ddy(input.worldPos.x));
    float2 dVy = float2(ddx(input.worldPos.z), ddy(input.worldPos.z));
    float magX = length(dVx);
    float magY = length(dVy);
    float2 dudv = float2(magX, magY);
    
    // Compute an LOD based on the derivative magnitude.
    float lod = calculateLOD(length(dudv));
    const float lodFactor = 7.0; // Adjust this factor for smoother transitions.
    float lodFloor = floor(lod);
    float lodFade = fract(lod);
    
    float lodScale = pow(lodFactor, lodFloor);
    float cellSizeCurrent = gridCellSize * lodScale;
    float cellSizeNext = cellSizeCurrent * lodFactor;
    
    // Adjust dudv to thicken the grid lines a bit.
    float2 adjustedDudv = dudv * 4.0;
    
    float alphaCurrent = calculateAlphaLod(cellSizeCurrent, adjustedDudv, {input.worldPos.x, input.worldPos.y, input.worldPos.z});
    float alphaNext = calculateAlphaLod(cellSizeNext, adjustedDudv, {input.worldPos.x, input.worldPos.y, input.worldPos.z});
    
    // Blend between two grid levels.
    float4 colorCurrent = gridColorThin;
    colorCurrent.a *= alphaCurrent * (1.0 - lodFade);
    
    float4 colorNext = gridColorThick;
    colorNext.a *= alphaNext * lodFade;
    
    float4 gridOut = colorCurrent + colorNext;
    gridOut.a = min(gridOut.a, 1.0);
    
    // Use our improved axis line drawing function with gridSize as far plane
    float4 axisColor = drawAxisLines({input.worldPos.x, input.worldPos.y, input.worldPos.z}, dudv, input.far);
    
    // Blend the axis lines over the grid
    if (axisColor.a > 0.0) {
        gridOut = lerp(gridOut, axisColor, axisColor.a);
    }
    
    FragmentOutput output;
    output.color = gridOut;
    return output;
}

