// Includes boilerplate code necessary for a screen shader
#include FRAME_UNIFORMS
#include SCREEN_PASS_VERTEX_SHADER

/* This struct is defined in SCREEN_PASS_VERTEX_SHADER
struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) v_uv : vec2<f32>,
}; */

// Specifies the entry point for the fragment shader.
// ChuGL requires the name fs_main for the entry point.
@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    // ChuGL unconditionally provides frame uniforms at group 0,
    // binding 0. If the uniforms are not referenced, they will
    // be optimized out, causing a mismatch between the bind group
    // layout and the uniforms provided by ChuGL. Referencing it
    // here prevents that from happening.
    let UNUSED = u_frame;

    // YOUR CODE HERE
    return vec4f(in.v_uv, 1.0, 1.0);
}