#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;


layout (location = 0) out vec4 outFragColor;

struct SHCoefficients {
    vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
};

const SHCoefficients grace = SHCoefficients(
    vec3( 0.3623915,  0.2624130,  0.2326261 ),
    vec3( 0.1759131,  0.1436266,  0.1260569 ),
    vec3(-0.0247311, -0.0101254, -0.0010745 ),
    vec3( 0.0346500,  0.0223184,  0.0101350 ),
    vec3( 0.0198140,  0.0144073,  0.0043987 ),
    vec3(-0.0469596, -0.0254485, -0.0117786 ),
    vec3(-0.0898667, -0.0760911, -0.0740964 ),
    vec3( 0.0050194,  0.0038841,  0.0001374 ),
    vec3(-0.0818750, -0.0321501,  0.0033399 )
);

vec3 calcIrradiance(vec3 nor) {
    const SHCoefficients c = grace;
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;
    return (
        c1 * c.l22 * (nor.x * nor.x - nor.y * nor.y) +
        c3 * c.l20 * nor.z * nor.z +
        c4 * c.l00 -
        c5 * c.l20 +
        2.0 * c1 * c.l2m2 * nor.x * nor.y +
        2.0 * c1 * c.l21  * nor.x * nor.z +
        2.0 * c1 * c.l2m1 * nor.y * nor.z +
        2.0 * c2 * c.l11  * nor.x +
        2.0 * c2 * c.l1m1 * nor.y +
        2.0 * c2 * c.l10  * nor.z
    );
}

// Makes the normal look better for little performance cost
vec3 calculateTangentGramSchmidt(in vec3 normal, in vec3 tangent) {
	return (tangent - dot(tangent, normal) * normal);
}

// Tangent, BiTangent and normal matrix
// Converts texture space into model space
mat3 TBN;

void main() 
{


	// Calculate normal related stuff
	vec3 tangent = calculateTangentGramSchmidt(inNormal, inTangent);
	vec3 biTangent = cross(inNormal, tangent);
	TBN = mat3(tangent, biTangent, inNormal);

	vec3 localNormal = 2.0 * texture(normalTex, inUV).rgb - 1.0;
	vec3 finalNormal = normalize(TBN * localNormal);
	//Calculate the light once we're done with normal calculations
	float lightValue = max(dot(finalNormal, vec3(0.3f,1.f,0.3f)), 0.1f);
	vec3 irradiance = calcIrradiance(finalNormal);

	vec3 color = inColor * texture(colorTex,inUV).xyz;

	outFragColor = vec4(color * lightValue + color * irradiance.x * vec3(0.2f) ,1.0f);
}

