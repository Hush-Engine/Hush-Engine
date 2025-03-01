#version 450

layout(location = 0) in float near; //01
layout(location = 1) in float gridSize; //10
layout(location = 2) in vec3 worldPos;
layout(location = 3) in vec3 farPoint;
layout(location = 4) in mat4 fragView;
layout(location = 8) in mat4 fragProj;
layout(location = 0) out vec4 outColor;


const float gridCellSize = 1.0;
const float minPixelsBetweenCells = 2.0;

const vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.);
const vec4 gridColorThick = vec4(0., 0., 0., 1.);

float log10(float x) {    
    return log(x) / log(10.0);
}

float max2(vec2 v) {
    return max(v.x, v.y);
}

vec2 satv(vec2 x) {
    return clamp(x, vec2(0.0), vec2(1.0));
}

float calculateAlphaLod(float gridLod, vec2 dudv) {
    vec2 moduloDiv = mod(worldPos.xz, gridLod) / dudv;
    return max2(vec2(1.0) - abs(satv(moduloDiv) * 2.0 - vec2(1.0)));
}

float calculateLOD(float derivativeMagnitude) {
    return max(0.0, log10(derivativeMagnitude * minPixelsBetweenCells / gridCellSize) + 1.0);
}


vec4 drawAxisLines(vec3 worldPos, float axisLineWidth) {
    vec4 axisColor = vec4(0.0);

    // X-axis (red), we had to invert it for... reasons(? I'm new to shaders
    if (abs(worldPos.z) < axisLineWidth) {
        axisColor = vec4(1.0, 0.0, 0.0, 1.0);
    }    
    
    else if (abs(worldPos.x) < axisLineWidth) {
        axisColor = vec4(0.0, 0.0, 1.0, 1.0);
    }

    return axisColor;
}

void main() {
    vec2 dVx = vec2(dFdx(worldPos.x), dFdy(worldPos.x));
    vec2 dVy = vec2(dFdx(worldPos.z), dFdy(worldPos.z));

    float magX = length(dVx);
    float magY = length(dVy);

    vec2 dudv = vec2(magX, magY);
    float lod = calculateLOD(length(dudv));
    const float lodFactor = 7.0; // Reduced from 10 for smoother transitions
    float lodFloor = floor(lod);
    float lodFade = fract(lod);

    float lodScale = pow(lodFactor, lodFloor);
    float cellSizeCurrent = gridCellSize * lodScale;
    float cellSizeNext = cellSizeCurrent * lodFactor;

    // Apply line thickness adjustment
    dudv *= 4.0;

 
    float alphaCurrent = calculateAlphaLod(cellSizeCurrent, dudv);
    float alphaNext = calculateAlphaLod(cellSizeNext, dudv);

 
    vec4 colorCurrent = gridColorThin;
    colorCurrent.a *= alphaCurrent * (1.0 - lodFade);

    vec4 colorNext = gridColorThick;
    colorNext.a *= alphaNext * lodFade;

 
    outColor = colorCurrent + colorNext;
    outColor.a = min(outColor.a, 1.0); // Clamp alpha

 
    float axisLineWidth = 0.5;
    vec4 axisColor = drawAxisLines(worldPos, axisLineWidth);

    
    if (axisColor.a > 0.0) {
        outColor = mix(outColor, axisColor, axisColor.a);
    }
}
