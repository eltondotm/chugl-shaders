#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(1) v_normal : vec3<f32>,
};

// Mostly STANDARD_VERTEX_SHADER but with reduced output
@vertex
fn vs_main(in : VertexInput) -> VertexOutput {
    var out : VertexOutput;
    var u_Draw : DrawUniforms = u_draw_instances[in.instance];

    let worldpos = u_Draw.model * vec4f(in.position, 1.0f);
    out.position = (u_frame.projection * u_frame.view) * worldpos;
    out.v_normal = (u_Draw.normal * vec4f(in.normal, 0.0)).xyz;

    return out;
}

// Encoding depth/normal information in rgb values
@fragment
fn fs_main(in : VertexOutput, @builtin(front_facing) front : bool) -> @location(0) vec4f {
    let UNUSED = u_frame;

    let normal = max(in.v_normal, vec3f(0.0));
    let latitude = atan2(normal.y, length(normal.xz));
    let longitude = atan2(normal.z, normal.x);
    let depth = in.position.z;

    return vec4f(latitude, longitude, depth, 1.0);
}