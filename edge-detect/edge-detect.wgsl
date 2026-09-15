// Boilerplate for a screen pass
#include FRAME_UNIFORMS
#include SCREEN_PASS_VERTEX_SHADER

// Uniforms: render texture, depth-normal texture, and sampler
@group(1) @binding(0) var render_texture : texture_2d<f32>;
@group(1) @binding(1) var buffer_texture : texture_2d<f32>;
@group(1) @binding(2) var texture_sampler : sampler;

@group(1) @binding(3) var<uniform> thresholds : vec2f;

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    let UNUSED = u_frame; 

    // Sampling the color from the render output
    let base_color = textureSample(render_texture, texture_sampler, in.v_uv);

    // Sampling the depth normal information
    let buff_color = textureSample(buffer_texture, texture_sampler, in.v_uv);
    let normal = buff_color.xy;
    let depth = log(buff_color.z);

    // Simple approach: using the magnitudes of partial derivatives of each
    // component of the depth-normal buffer. Note that this is dependent on
    // resolution because fwidth acts on adjacent fragments. A good alternative
    // would be to use the sobel operator at fixed UV offsets.
    let edge_normal = fwidth(normal);
    let edge_depth = fwidth(depth);

    if (edge_depth > thresholds.x) {
        return vec4f(0.0, 1.0, 0.0, 1.0);
    }

    if (dot(edge_normal, edge_normal) > thresholds.y) {
        return vec4f(1.0, 0.0, 1.0, 1.0);
    }
    
    return base_color;
}