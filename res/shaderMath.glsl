#ifndef SHADER_MATH_H
#define SHADER_MATH_H
const float Epsilon = 0.00001;


// Makes the normal look better for little performance cost
vec3 calculateTangentGramSchmidt(in vec3 normal, in vec3 tangent) {
    return (tangent - dot(tangent, normal) * normal);
}

vec3 viewMatExtractFwd(in mat4 viewMatrix) {
    // 8 9 and 10 idx corresponds to -fwd
    return vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
}

vec3 viewMatExtractPos(in mat4 viewMatrix) {
    // I... think this is correct, we need to negate this
    return -vec3(viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]);
}

vec3 scalarPow(in vec3 v, in float n) {
    return vec3(pow(v.x, n), pow(v.y, n), pow(v.z, n));
}


vec4 ToLinear(vec4 sRGB)
{
	bvec4 cutoff = lessThan(sRGB, vec4(0.04045));
	vec4 higher = pow((sRGB + vec4(0.055))/vec4(1.055), vec4(2.4));
	vec4 lower = sRGB/vec4(12.92);

	return mix(higher, lower, cutoff);
}

bool HasCompositeFlag(int base, int compositeFlag) {
	return (base & compositeFlag) != 0;
}


#endif
