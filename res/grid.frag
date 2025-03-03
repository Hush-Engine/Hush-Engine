#version 450
layout(location = 0) in float near;      // e.g. 0.1
layout(location = 1) in float gridSize;  // Far clipping plane distance
layout(location = 2) in vec3 worldPos;
layout(location = 3) in vec3 farPoint;
layout(location = 4) in mat4 fragView;
layout(location = 8) in mat4 fragProj;
layout(location = 0) out vec4 outColor;
const float gridCellSize = 1.0;
const float minPixelsBetweenCells = 2.0;
const vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);
const vec4 gridColorThick = vec4(0.0, 0.0, 0.0, 1.0);
const vec4 xAxisColor = vec4(1.0, 0.0, 0.0, 1.0);
const vec4 zAxisColor = vec4(0.0, 0.0, 1.0, 1.0);

float log10(float x) {
    return log(x) / log(10.0);
}

float max2(vec2 v) {
    return max(v.x, v.y);
}

vec2 satv(vec2 x) {
    return clamp(x, vec2(0.0), vec2(1.0));
}

// Given the current worldPos and a chosen grid cell size (gridLod),
// calculate an alpha value for the grid line based on how close we are to a cell edge.
// dudv is a screen-space estimate (via derivatives) of world-space change.
float calculateAlphaLod(float gridLod, vec2 dudv) {
    vec2 moduloDiv = mod(worldPos.xz, gridLod) / dudv;
    return max2(vec2(1.0) - abs(satv(moduloDiv) * 2.0 - vec2(1.0)));
}

// Compute a LOD value based on the magnitude of the derivative.
float calculateLOD(float derivativeMagnitude) {
    return max(0.0, log10(derivativeMagnitude * minPixelsBetweenCells / gridCellSize) + 1.0);
}


vec4 drawAxisLines(vec3 fragPos3D, vec2 dudv, float farPlane) {
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
        return vec4(xAxisColor.rgb, alphaX);
    } else if (alphaZ > 0.0) {
        return vec4(zAxisColor.rgb, alphaZ);
    }
    return vec4(0.0);
}

void main() {
    // Compute screen-space derivatives for the x and z worldPos components.
    vec2 dVx = vec2(dFdx(worldPos.x), dFdy(worldPos.x));
    vec2 dVy = vec2(dFdx(worldPos.z), dFdy(worldPos.z));
    float magX = length(dVx);
    float magY = length(dVy);
    vec2 dudv = vec2(magX, magY);
    
    // Compute an LOD based on the derivative magnitude.
    float lod = calculateLOD(length(dudv));
    const float lodFactor = 7.0; // Adjust this factor for smoother transitions.
    float lodFloor = floor(lod);
    float lodFade = fract(lod);
    
    float lodScale = pow(lodFactor, lodFloor);
    float cellSizeCurrent = gridCellSize * lodScale;
    float cellSizeNext = cellSizeCurrent * lodFactor;
    
    // Adjust dudv to thicken the grid lines a bit.
    vec2 adjustedDudv = dudv * 4.0;
    
    float alphaCurrent = calculateAlphaLod(cellSizeCurrent, adjustedDudv);
    float alphaNext = calculateAlphaLod(cellSizeNext, adjustedDudv);
    
    // Blend between two grid levels.
    vec4 colorCurrent = gridColorThin;
    colorCurrent.a *= alphaCurrent * (1.0 - lodFade);
    
    vec4 colorNext = gridColorThick;
    colorNext.a *= alphaNext * lodFade;
    
    vec4 gridOut = colorCurrent + colorNext;
    gridOut.a = min(gridOut.a, 1.0);
    
    // Use our improved axis line drawing function with gridSize as far plane
    vec4 axisColor = drawAxisLines(worldPos, dudv, gridSize);
    
    // Blend the axis lines over the grid
    if (axisColor.a > 0.0) {
        gridOut = mix(gridOut, axisColor, axisColor.a);
    }
    
    outColor = gridOut;
}
