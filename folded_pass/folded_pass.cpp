#pragma once

#include <iostream>
#include <fstream>

#include <string>
#include <vector>

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
    std::vector<Pass> passes = { vignette };
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
