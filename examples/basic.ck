// Specifies shader properties
// All screen shader descs will have this format, but with a different filename
ShaderDesc desc;
me.dir() + "basic.wgsl" => desc.vertexPath;
me.dir() + "basic.wgsl" => desc.fragmentPath;
null => desc.vertexLayout;

// Create a Shader component containing our WGSL code
Shader basicShader(desc);
basicShader.name("Screen Shader");

// Assign the shader to a screen pass (graphics pipeline applied to the full screen)
ScreenPass basic(basicShader);

// Rewire the render graph to only contain our screen shader (ignoring the scene)
GG.rootPass() --> basic;

// Main loop
while (true)
{
    GG.nextFrame() => now;
}