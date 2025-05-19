#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out float outHandedness;
layout (location = 5) out vec3 outWorldPos;

struct Vertex {
	vec3 position;
	vec3 normal;
	vec4 color;
	vec4 tangent;
	vec2 uv;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{
	mat4 modelMatrix;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	
	vec4 position = vec4(v.position, 1.0f);

	gl_Position =  u_sceneData.viewproj * PushConstants.modelMatrix * position;
	outWorldPos = (PushConstants.modelMatrix * vec4(v.position, 1.0f)).xyz;
	
	mat3 modelMat3 = mat3(PushConstants.modelMatrix);
	outNormal = normalize(modelMat3 * v.normal);
	outTangent = normalize(modelMat3 * v.tangent.xyz);
	outHandedness = v.tangent.w;
	outColor = v.color.xyz * materialData.colorFactors.xyz;	
	
	outUV.x = v.uv.x;
	outUV.y = v.uv.y;
}

