#include FRAME_UNIFORMS
// what's included:
// struct FrameUniforms {
//     // scene params (only set in ScenePass, otherwise 0)
//     projection: mat4x4f,
//     view: mat4x4f,
//     projection_view_inverse_no_translation: mat4x4f,
//     camera_pos: vec3f,
//     ambient_light: vec3f,
//     num_lights: i32,
//     background_color: vec4f,

//     // general params (include in all passes except ComputePass)
//     resolution: vec3i,      // window viewport resolution 
//     time: f32,              // time in seconds since the graphics window was opened
//     delta_time: f32,        // time since last frame (in seconds)
//     frame_count: i32,       // frames since window was opened
//     mouse: vec2f,           // normalized mouse coords (range 0-1)
//     mouse_click: vec2i,     // mouse click state
//     sample_rate: f32        // chuck VM sound sample rate (e.g. 44100)
// };
// @group(0) @binding(0) var<uniform> u_frame: FrameUniforms;

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) v_uv : vec2<f32>,
};

fn posterize(a : f32, cutoffs : vec3f) -> vec4f {
    let c0 = vec4f(29, 53, 87, 255) / 255.0;
    let c1 = vec4f(69, 123, 157, 255) / 255.0;
    let c2 = vec4f(168, 218, 220, 255) / 255.0;
    let c3 = vec4f(241, 250, 238, 255) / 255.0;

    if (a < cutoffs.x) { return c0; }
    if (a < cutoffs.y) { return c1; }
    if (a < cutoffs.z) { return c2; }
    return c3;
}

// SHADER UNIFORMS
@group(1) @binding(0) var render_texture : texture_2d<f32>;
@group(1) @binding(1) var render_sampler : sampler;

@group(1) @binding(2) var<uniform> cutoffs : vec3f;


// VERTEX SHADER (boilerplate for any screen shader)
@vertex 
fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
    var output : VertexOutput;

    // a triangle which covers the screen
    output.v_uv = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u));
    output.position = vec4f(output.v_uv * 2.0 - 1.0, 0.0, 1.0);
    
    return output;
}


// FRAGMENT SHADER
@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    let t0 = u_frame; 
    let uv = vec2f(in.v_uv.x, 1.0 - in.v_uv.y);

    // sampling the color from the render output
    let base_color = textureSample(render_texture, render_sampler, uv);

    // converting to grayscale
    let value = 0.2125*base_color.r + 0.7154*base_color.g + 0.0721*base_color.b;
    
    return posterize(value, cutoffs);
}