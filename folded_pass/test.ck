
me.dir() + "assets/" => string asset_path;

TextureLoadDesc load_desc;
true => load_desc.flip_y;
true => load_desc.gen_mips;
Texture.load(asset_path + "leo.JPG", load_desc) @=> Texture image;
image.width() $ float / image.height() $ float => float aspect;

TextureLoadDesc atlas_load_desc;
false => atlas_load_desc.flip_y;
false => atlas_load_desc.gen_mips;  // mips incorrectly blend slices
Texture.load(asset_path + "saturated.png", atlas_load_desc) @=> Texture lut;

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
folded_pass.material().uniformInt(2, 1);       // gamma
folded_pass.material().uniformFloat(3, 1.0);   // exposure
folded_pass.material().uniformInt(4, 0);       // tonemap
folded_pass.material().texture(5, lut);        // lut texture
folded_pass.material().sampler(6, TextureSampler.linear());
folded_pass.material().uniformFloat(7, 0.2);   // vignette radius
folded_pass.material().uniformFloat(8, 0.6);   // vignette strength

while (true)
{
    GG.nextFrame() => now;
}
