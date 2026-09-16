
#include FRAME_UNIFORMS
#include SCREEN_PASS_VERTEX_SHADER

@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var texture_sampler: sampler;
@group(1) @binding(2) var<uniform> vignette_size: f32;
@group(1) @binding(3) var<uniform> vignette_strength: f32;

fn vignette(color: vec4f, uv: vec2f) -> vec4f {
    let radius = distance(uv, vec2f(0.5));
    let val = smoothstep(0.5 - vignette_size,
                         0.5 + vignette_size,
                         radius) * vignette_strength;
    return mix(color, vec4f(0, 0, 0, 1), val);
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    let frame = u_frame; 
    var color = textureSample(texture, texture_sampler, in.v_uv);
	color = vignette(color, in.v_uv);
	return color;
}
    