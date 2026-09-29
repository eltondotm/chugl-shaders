#pragma once

#include <iostream>
#include <fstream>

#include <string>
#include <vector>

/* USAGE: .\folded_pass [passes]
 * EXAMPLE: .\folded_pass output lut vignette
 * Current options:
 *   vignette
 *   aberration
 *   lut
 *   output
 *
 * TODO:
 * Handle multiple instances of the same effect
 * Replace uniform names on naming collision
 * Generate ChuGL uniform binding code as well
 * Read passes from files instead of string literals
 */

enum Type 
{
    F32,
    VEC2F,
    VEC3F,
    VEC4F,
    I32,
    VEC2I,
    VEC3I,
    VEC4I,
    TEXTURE_2D,
    SAMPLER
};

struct Uniform
{
    const std::string name;
    const Type type;
};

struct Pass 
{
    const std::string name;
    const std::string fn;
    std::vector<Uniform> uniforms;
};

static const char* shader_base = R"wgsl(
#include FRAME_UNIFORMS
#include SCREEN_PASS_VERTEX_SHADER

@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var texture_sampler: sampler;

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    let frame = u_frame; 
    var color = textureSample(texture, texture_sampler, in.v_uv);
)wgsl";

static const Pass vignette = 
{
    "vignette",
    R"wgsl(
fn vignette(color: vec4f, uv: vec2f) -> vec4f {
    let radius = distance(uv, vec2f(0.5));
    let val = smoothstep(0.5 - vignette_size,
                         0.5 + vignette_size,
                         radius) * vignette_strength;
    return mix(color, vec4f(0, 0, 0, 1), val);
}
    )wgsl",
    {
        { "vignette_size",  F32 },
        { "vignette_strength", F32 }
    }
};

static const Pass aberration =
{
    "aberration",
    R"wgsl(
fn aberration(color: vec4f, uv: vec2f) -> vec4f
{
    let r_u = max(uv.x - aberration_width, 0.0);
    let b_u = min(uv.x + aberration_width, 1.0);

    let r = textureSample(texture, texture_sampler, vec2f(r_u, uv.y)).r;
    let g = color.g;
    let b = textureSample(texture, texture_sampler, vec2f(b_u, uv.y)).b;
    
    return vec4f(r, g, b, 1.0);
}
    )wgsl",
    {
        { "aberration_width", F32 }
    }
};

static const Pass lut =
{
    "lut",
    R"wgsl(
// https://webgpufundamentals.org/webgpu/lessons/webgpu-3dlut.html
// https://webgpufundamentals.org/webgpu/lessons/webgpu-3dlut.html
fn lut(color: vec4f, uv: vec2f) -> vec4f 
{
    let size_flat = vec2f(textureDimensions(lut_texture, 0));
    let depth = size_flat.x / size_flat.y;
    let size = vec3f(size_flat.y, size_flat.y, depth);
    let range = (size - 1.0) / size;
    let uvw = 0.5 / size + color.rgb * range;
  
    // manual interpolation while 3D texture loading unsupported
    let slice_coord = uvw.z * depth;
    let w_l = floor(slice_coord);
    let w_r = ceil(slice_coord);
    let t = fract(slice_coord);

    let uv_l = vec2f((w_l + uvw.x) / depth, uvw.y);
    let uv_r = vec2f((w_r + uvw.x) / depth, uvw.y);

    let col_l = textureSample(lut_texture, lut_sampler, uv_l);
    let col_r = textureSample(lut_texture, lut_sampler, uv_r);
    return mix(col_l, col_r, t);
}
    )wgsl",
    {
        { "lut_texture", TEXTURE_2D },
        { "lut_sampler", SAMPLER },
    }
};

static const Pass output =
{
    "output",
    R"wgsl(
const TONEMAP_NONE = 0;
const TONEMAP_LINEAR = 1;
const TONEMAP_REINHARD = 2;
const TONEMAP_CINEON = 3;
const TONEMAP_ACES = 4;
const TONEMAP_UNCHARTED = 5;

// Helpers ==================================================================
fn Uncharted2Tonemap(x: vec3<f32>) -> vec3<f32> {
    let A: f32 = 0.15;
    let B: f32 = 0.5;
    let C: f32 = 0.1;
    let D: f32 = 0.2;
    let E: f32 = 0.02;
    let F: f32 = 0.3;
    return (x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F) - E / F;
} 

// source: https://github.com/selfshadow/ltc_code/blob/master/webgl/shaders/ltc/ltc_blit.fs
fn rrt_odt_fit(v: vec3<f32>) -> vec3<f32> {
    let a: vec3<f32> = v * (v + 0.0245786) - 0.000090537;
    let b: vec3<f32> = v * (0.983729 * v + 0.432951) + 0.238081;
    return a / b;
} 

fn mat3_from_rows(c0: vec3<f32>, c1: vec3<f32>, c2: vec3<f32>) -> mat3x3<f32> {
    var m: mat3x3<f32> = mat3x3<f32>(c0, c1, c2);
    m = transpose(m);
    return m;
} 

// from https://medium.com/@tomforsyth/the-srgb-learning-curve-773b7f68cf7a
fn D3DX_FLOAT_to_SRGB(val: f32) -> f32
{ 
    var v = val;
    if(v < 0.0031308) {
        v *= 12.92;
    } else {
        v = 1.055 * pow(val, 1.0/2.4) - 0.055;
    }
    return v;
}

// main =====================================================================
fn output(color: vec4f, uv: vec2f) -> vec4f {
    let hdrColor: vec4<f32> = color;
    var out_color: vec3<f32> = hdrColor.rgb;
    if (u_Tonemap != TONEMAP_NONE) {
        out_color = out_color * (u_Exposure);
    }
    switch (u_Tonemap) {
    case 1: { // linear
        out_color = clamp(out_color, vec3f(0.), vec3f(1.));
    }
    case 2: { // reinhard
        out_color = hdrColor.rgb / (hdrColor.rgb + vec3<f32>(1.));
    }
    case 3: { // cineon
        let x: vec3<f32> = max(vec3<f32>(0.), out_color - 0.004);
        out_color = x * (6.2 * x + 0.5) / (x * (6.2 * x + 1.7) + 0.06);
        out_color = pow(out_color, vec3<f32>(2.2)); // invert gamma correction (assumes final output to srgb texture)
    } 
    case 4: { // aces
        var ACES_INPUT_MAT: mat3x3<f32> = mat3_from_rows(vec3<f32>(0.59719, 0.35458, 0.04823), vec3<f32>(0.076, 0.90834, 0.01566), vec3<f32>(0.0284, 0.13383, 0.83777));
        var ACES_OUTPUT_MAT: mat3x3<f32> = mat3_from_rows(vec3<f32>(1.60475, -0.53108, -0.07367), vec3<f32>(-0.10208, 1.10813, -0.00605), vec3<f32>(-0.00327, -0.07276, 1.07602));
        out_color = out_color / 0.6;
        out_color = ACES_INPUT_MAT * out_color;
        out_color = rrt_odt_fit(out_color);
        out_color = ACES_OUTPUT_MAT * out_color;
        out_color = clamp(out_color, vec3f(0.), vec3f(1.));
    }
    case 5: { // uncharted
        let ExposureBias: f32 = 2.;
        let curr: vec3<f32> = Uncharted2Tonemap(ExposureBias * out_color);
        let W: f32 = 11.2;
        let whiteScale: vec3<f32> = vec3<f32>(1. / Uncharted2Tonemap(vec3<f32>(W)));
        out_color = curr * whiteScale;
    }
    default: {}
    }

    // gamma correction
    // out_color = pow(out_color, vec3<f32>(1. / u_Gamma));
    if (bool(u_Gamma)) {
        out_color.r = D3DX_FLOAT_to_SRGB(out_color.r);
        out_color.g = D3DX_FLOAT_to_SRGB(out_color.g);
        out_color.b = D3DX_FLOAT_to_SRGB(out_color.b);
    }

    return vec4<f32>(out_color, 1.0); // how does alpha work?
    // return vec4<f32>(out_color, clamp(hdrColor.a, 0.0, 1.0));
}
    )wgsl",
    {
        { "u_Gamma", I32 },
        { "u_Exposure", F32 },
        { "u_Tonemap", I32 }
    }
};

std::string gen_uniform_decl(const Uniform& u, int binding)
{
    std::string decl = "@group(1) @binding(" + std::to_string(binding) + ") ";
    switch (u.type)
    {
        case F32:   decl += "var<uniform> " + u.name + ": f32;";   break;
        case VEC2F: decl += "var<uniform> " + u.name + ": vec2f;"; break;
        case VEC3F: decl += "var<uniform> " + u.name + ": vec3f;"; break;
        case VEC4F: decl += "var<uniform> " + u.name + ": vec4f;"; break;
        case I32:   decl += "var<uniform> " + u.name + ": i32;";   break;
        case VEC2I: decl += "var<uniform> " + u.name + ": vec2i;"; break;
        case VEC3I: decl += "var<uniform> " + u.name + ": vec3i;"; break;
        case VEC4I: decl += "var<uniform> " + u.name + ": vec4i;"; break;
        case TEXTURE_2D: decl += "var " + u.name + ": texture_2d<f32>;"; break;
        case SAMPLER:    decl += "var " + u.name + ": sampler;";         break;
        default: 
            std::cerr << "Invalid uniform type: " 
                      << std::to_string(u.type)
                      << std::endl;
            return "";
    }
    return decl;
}

std::string gen_folded_pass(const std::vector<Pass>& passes)
{
    // Add pass functions to fragment shader
    std::string folded_pass = shader_base;
    for (const Pass& pass : passes)
    {
        folded_pass += "\tcolor = " + pass.name + "(color, in.v_uv);\n";
    }
    folded_pass += "\treturn color;\n}\n";

    // Function and uniform declarations
    int binding = 2;
    for (const Pass& pass : passes)
    {
        for (const Uniform& u : pass.uniforms)
        {
            folded_pass += "\n" + gen_uniform_decl(u, binding);
            binding++;
        }
        folded_pass += "\n" + pass.fn;
    }
    return folded_pass;
}

int main(int argc, char* argv[])
{
    std::vector<Pass> passes;
    if (argc == 1)
    {
        // Default passes
        passes.push_back(lut);
        passes.push_back(output);
        passes.push_back(vignette);
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg == "vignette")        { passes.push_back(vignette); }
            else if (arg == "aberration") { passes.push_back(aberration); }
            else if (arg == "lut")        { passes.push_back(lut); }
            else if (arg == "output")     { passes.push_back(output); }
            else { std::cerr << "Invalid pass: " << arg << std::endl; }
        }
    }
    std::string folded_pass = gen_folded_pass(passes);

    std::ofstream pass_file("folded_pass.wgsl");
    if (pass_file.is_open())
    {
        pass_file << folded_pass;
        pass_file.close();
        std::cout << "Folded pass written to folded_pass.wgsl." << std::endl;
    } 
    else
    {
        std::cerr << "Failed to create file." << std::endl;
    }

    return 0;
}
