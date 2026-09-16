
Texture.load(me.dir() + "leo.JPG") @=> Texture image;
image.width() $ float / image.height() $ float => float aspect;

FlatMaterial plane_mat;
plane_mat.colorMap(image);
PlaneGeometry plane_geo;
GMesh cam_plane(plane_geo, plane_mat) --> GG.scene();

cam_plane.scaX(aspect);

// Match window size to image
GWindow.title("Folded Pass Test");
GWindow.windowed((1080 * aspect) $ int, 1080);

// Aligning camera with webcam plane
GG.camera().orthographic();
GG.camera().viewSize(1.0);

// Shader setup
ShaderDesc folded_desc;
me.dir() + "folded_pass.wgsl" => folded_desc.vertexPath;
me.dir() + "folded_pass.wgsl" => folded_desc.fragmentPath;
null => folded_desc.vertexLayout;

Shader folded(folded_desc);
folded.name("folded-pass");

// Create a new pass following the scene pass
GG.scenePass() --> ScreenPass folded_pass(folded);

// Set shader uniforms
folded_pass.material().texture(0, GG.renderPass().colorOutput());
folded_pass.material().sampler(1, TextureSampler.linear());
folded_pass.material().uniformFloat(2, 0.2);
folded_pass.material().uniformFloat(3, 1.0);

while (true)
{
    GG.nextFrame() => now;
}
