struct Uniforms {
    mvp: mat4x4<f32>,
    tintColor: vec4<f32>,
    time: f32,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var tex: texture_2d<f32>;
@group(0) @binding(2) var texSampler: sampler;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

// Centered half-size quad as two triangles (6 vertices, CCW winding)
const kPositions = array<vec2<f32>, 6>(
    vec2<f32>(-0.5,  0.5),   // top-left
    vec2<f32>(-0.5, -0.5),   // bottom-left
    vec2<f32>( 0.5, -0.5),   // bottom-right
    vec2<f32>(-0.5,  0.5),   // top-left
    vec2<f32>( 0.5, -0.5),   // bottom-right
    vec2<f32>( 0.5,  0.5),   // top-right
);

const kUVs = array<vec2<f32>, 6>(
    vec2<f32>(0.0, 0.0),     // top-left
    vec2<f32>(0.0, 1.0),     // bottom-left
    vec2<f32>(1.0, 1.0),     // bottom-right
    vec2<f32>(0.0, 0.0),     // top-left
    vec2<f32>(1.0, 1.0),     // bottom-right
    vec2<f32>(1.0, 0.0),     // top-right
);

@vertex
fn vertexMain(@builtin(vertex_index) vertexID: u32) -> VertexOutput {
    var output: VertexOutput;
    output.position = vec4<f32>(kPositions[vertexID], 0.0, 1.0);
    output.uv = kUVs[vertexID];
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(tex, texSampler, input.uv);
}
