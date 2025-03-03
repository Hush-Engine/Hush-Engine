#version 450

layout(set = 0, binding = 0) uniform ViewUniforms {
    mat4 viewproj;
    vec3 pos;
    float farPlane;
} view;

layout(location = 0) out float near;
layout(location = 1) out float gridSize;
layout(location = 2) out vec3 worldPos;
layout(location = 3) out vec3 farPoint;
layout(location = 4) out mat4 fragView;
layout(location = 8) out mat4 fragProj;

const vec3 pos[4] = vec3[4](
    vec3(-1.0, 0.0, -1.0),
    vec3(1.0, 0.0, -1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(-1.0, 0.0, 1.0)
);

const int indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main() {    
    int idx = indices[gl_VertexIndex];
    // Offset by the camera's world pos and the scale (model)
    vec3 vertPos = pos[idx] * view.farPlane;
    vertPos.x += view.pos.x;
    vertPos.z += view.pos.z;
    worldPos = vertPos;
    worldPos.y = view.pos.y;
    
    // Extend to vec4 and transform to viewproj 
    vec4 vertPos4 = vec4(vertPos, 1.0);
    gl_Position = view.viewproj * vertPos4;
    gridSize = view.farPlane;
}
