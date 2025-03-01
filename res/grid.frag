#version 450

layout(location = 0) in float near; //0.01
layout(location = 1) in float gridSize; //100
layout(location = 2) in vec3 worldPos;
layout(location = 3) in vec3 farPoint;
layout(location = 4) in mat4 fragView;
layout(location = 8) in mat4 fragProj;
layout(location = 0) out vec4 outColor;


const float gridCellSize = 0.025;

const float minPixelsBetweenCells = 2.0;

const vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.);
const vec4 gridColorThick = vec4(0., 0., 0., 1.);


float log10(float x) {    
    float f = log(x) / log(10.0);
    return f;
}

float max2(vec2 v)
{
    float f = max(v.x, v.y);
    return f;
}


vec2 satv(vec2 x)
{
    vec2 v = clamp(x, vec2(0.0), vec2(1.0));
    return v;
}

float calculateAlphaLod(float gridLod, vec2 dudv) {
    vec2 moduloDiv = mod(worldPos.xz, gridLod) / dudv;
    return max2(vec2(1.0) - abs(satv(moduloDiv) * 2.0 - vec2(1.0)));
}

float calculateLOD(float derivativeMagnitude) {
    return max(0.0, log10(derivativeMagnitude * minPixelsBetweenCells / gridCellSize) + 1.0);
}

vec4 calculateGridLineColor(float alphaLod2, float alphaLod1, float fadeFactor) {
    if (alphaLod2 > 0.0) {
        return gridColorThick;
    }
    if (alphaLod1 > 0.0) {
        return mix(gridColorThick, gridColorThin, fadeFactor);
    }
    return gridColorThin;
}

void main() {
    vec2 dVx = vec2(dFdx(worldPos.x), dFdy(worldPos.x));
    vec2 dVy = vec2(dFdx(worldPos.z), dFdy(worldPos.z));

    float magX = length(dVx);
    float magY = length(dVy);

    vec2 dudv = vec2(magX, magY);
    float lod = calculateLOD(length(dudv));
    float cellLod0 = gridCellSize * pow(10.0, floor(lod));
    float cellLod1 = cellLod0 * 10.0;
    float cellLod2 = cellLod1 * 10.0;
    // Arbitrary value to make the lines a but thicker and avoid aliasing
    dudv *= 4.0;
    float lod0a = calculateAlphaLod(cellLod0, dudv);
    float lod1a = calculateAlphaLod(cellLod1, dudv);
    float lod2a = calculateAlphaLod(cellLod2, dudv);

    float lodFade = fract(lod);
    
    // Calculate which line should be drawn depending on the LOD, and mix them in
    vec4 color = calculateGridLineColor(lod2a, lod1a, lodFade);
    color.a *= lod0a;
    
    outColor = color;
}
