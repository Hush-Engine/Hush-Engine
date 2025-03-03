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

// Draw axis lines with consistent thickness
vec4 drawAxisLines(vec3 fragPos3D, vec2 dudv, float farPlane) {
    // Calculate distance to x-axis (z=0) and z-axis (x=0)
    float distToXAxis = abs(fragPos3D.z);
    float distToZAxis = abs(fragPos3D.x);
    
    // Get world space distance to view origin (camera position)
    float distFromOrigin = length(fragPos3D.xz);
    
    // Base width calculation adjusted for near and far planes
    float pixelWorldRatio = length(dudv) / 2.0;
    float basePixelWidth = 3.0; // Desired width in pixels
    float baseWidth = pixelWorldRatio * basePixelWidth;
    
    // Scale up the width based on distance to maintain visual consistency
    // Using the ratio of current distance to far plane
    float distanceRatio = distFromOrigin / farPlane;
    
    // Use a curve that scales width appropriately to account for perspective
    // The exponent value controls how rapidly the line width increases with distance
    float scaleExponent = 1.5; // Adjust between 1.0-2.0 for different scaling behaviors
    float widthScale = pow(1.0 + distanceRatio, scaleExponent);
    
    // Apply the scaling with appropriate limits
    float lineWidth = baseWidth * widthScale;
    
    // Ensure we don't exceed reasonable width values
    float maxWidth = farPlane * 0.001;
    lineWidth = min(lineWidth, maxWidth);
    
    // Create smooth anti-aliased lines using smoothstep
    float xAxisStrength = 1.0 - smoothstep(0.0, lineWidth, distToXAxis);
    float zAxisStrength = 1.0 - smoothstep(0.0, lineWidth, distToZAxis);
    
    // Create the axis color 
    vec4 axisColor = vec4(0.0);
    
    // Apply strengths to respective color channels
    if (zAxisStrength > 0.0) {
        axisColor = mix(axisColor, xAxisColor, zAxisStrength);
    }
    
    if (xAxisStrength > 0.0) {
        axisColor = mix(axisColor, zAxisColor, xAxisStrength);
    }
    
    // Set the alpha based on whether either axis is visible
    axisColor.a = max(xAxisStrength, zAxisStrength);
    
    return axisColor;
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
