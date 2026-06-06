
// Webcam setup from basic/webcam.ck
Webcam webcam(0, 1280, 720, 60);

FlatMaterial cam_mat;
cam_mat.colorMap(webcam.texture());
PlaneGeometry cam_geo;
GMesh cam_plane(cam_geo, cam_mat) --> GG.scene();

cam_plane.scaX(webcam.aspect());

// Match window size to webcam resolution
GWindow.title("Posterize");
GWindow.windowed(webcam.width(), webcam.height());

// Aligning camera with webcam plane
GG.camera().orthographic();
GG.camera().viewSize(1.0);

// Shader setup
ShaderDesc poster_desc;
me.dir() + "posterize.wgsl" => poster_desc.vertexPath;
me.dir() + "posterize.wgsl" => poster_desc.fragmentPath;
null => poster_desc.vertexLayout;

Shader poster(poster_desc);
poster.name("poster");

// Create a new pass following the scene pass
GG.scenePass() --> ScreenPass poster_pass(poster);

// Initialize default cutoffs for each level
@(0.25, 0.5, 0.75) @=> vec3 cutoffs;

// Set shader uniforms
poster_pass.material().texture(0, GG.renderPass().colorOutput());
poster_pass.material().sampler(1, TextureSampler.linear());
poster_pass.material().uniformFloat3(2, cutoffs);

// Initialize UI vars
UI_Float c1(cutoffs.x);
UI_Float c2(cutoffs.y);
UI_Float c3(cutoffs.z);

while (true)
{
    GG.nextFrame() => now;

    if (UI.begin("Posterization Example")) {
        false => int update;  // Using a single if statement causes disapearring sliders
        if (UI.slider("Cutoff 1", c1, 0.0, 1.0)) { c1.val() => cutoffs.x; true => update; }
        if (UI.slider("Cutoff 2", c2, 0.0, 1.0)) { c2.val() => cutoffs.y; true => update; }
        if (UI.slider("Cutoff 3", c3, 0.0, 1.0)) { c3.val() => cutoffs.z; true => update; }
        
        if (update) { poster_pass.material().uniformFloat3(2, cutoffs); }
    }
}