#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out vec3 outVertPos;

struct Vertex {
	vec3 position;
	vec2 uv;
	vec3 normal;
	vec4 color;
	vec3 tangent;
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

	gl_Position =  sceneData.viewproj * PushConstants.modelMatrix *position;	

	
	outNormal = (PushConstants.modelMatrix * vec4(v.normal, 0.f)).xyz;
	outTangent = normalize(PushConstants.modelMatrix * v.tangent);
	outColor = v.color.xyz * materialData.colorFactors.xyz;	
	
	outUV.x = v.uv.x;
	outUV.y = v.uv.y;
	outVertPos = v.position;
}

