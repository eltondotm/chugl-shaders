/* name: edge-detect.ck */

// Create a new scene to render a depth/normal buffer (not built-in to ChuGL)
GScene buffScene;

// Switch to an orbit camera to look around
GOrbitCamera camera => GG.scene().camera;
GOrbitCamera buffCamera => buffScene.camera;

// Create a new texture for the buffer
TextureDesc bufferDesc;
Texture.FORMAT_RGBA16FLOAT => bufferDesc.format;
true => bufferDesc.resizable;
Texture.USAGE_RENDER_ATTACHMENT | Texture.USAGE_TEXTURE_BINDING 
    => bufferDesc.usage;

Texture buffer(bufferDesc);

// Create a pass to render the buffer, and output to the texture
ScenePass buffPass(buffScene);
buffPass.colorOutput(buffer);

// Custom depth-normal material to render our buffer
ShaderDesc depthNormalDesc;
me.dir() + "depth-normal.wgsl" => depthNormalDesc.vertexPath;
me.dir() + "depth-normal.wgsl" => depthNormalDesc.fragmentPath;

Shader depthNormalShader(depthNormalDesc);
depthNormalShader.name("depth normal");

// Postprocessing shader to apply outline effect
ShaderDesc outlineDesc;
me.dir() + "edge-detect.wgsl" => outlineDesc.vertexPath;
me.dir() + "edge-detect.wgsl" => outlineDesc.fragmentPath;
null => outlineDesc.vertexLayout;

Shader outlineShader(outlineDesc);
outlineShader.name("outline");

// Initializing parameters to tune depth/normal sensitivity
@(0.5, 0.5) @=> vec2 thresholds;

// Creating the outline pass and binding textures
ScreenPass outlinePass(outlineShader);
outlinePass.material().texture(0, GG.scenePass().colorOutput());
outlinePass.material().texture(1, buffer);
outlinePass.material().sampler(2, TextureSampler.linear());
outlinePass.material().uniformFloat2(3, thresholds*0.01);

// Wire the new passes into the render graph
GG.scenePass() --> buffPass --> outlinePass;

Material depthNormal(depthNormalShader);
PhongMaterial default;
SuzanneGeometry suzanne;

GMesh model[2];
for (0 => int i; i < model.size(); i++)
{
    GMesh mesh @=> model[i];
    model[i].geometry(suzanne);
    model[i].sca(2.0);
}
model[0].material(default);
model[0] --> GG.scene();
model[1].material(depthNormal);
model[1] --> buffScene;

// Initialize UI vars
UI_Float c1(thresholds.x);
UI_Float c2(thresholds.y);

while (true)
{
    GG.nextFrame() => now;

    if (UI.begin("Posterization Example")) {
        false => int update;  // Using a single if statement causes disapearring sliders
        if (UI.slider("Depth Threshold", c1, 0.0, 1.0)) { c1.val() => thresholds.x; true => update; }
        if (UI.slider("Normal Threshold", c2, 0.0, 1.0)) { c2.val() => thresholds.y; true => update; }
        
        if (update) { outlinePass.material().uniformFloat2(3, thresholds*0.01); }
    }
}