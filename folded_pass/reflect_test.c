#define CHUGL_WGSL_REFLECT_IMPLEMENTATION
#include "chugl_wgsl_reflect.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Please provide a file to parse.\n");
        return 0;
    }

    char *wgsl_path = argv[1];
    ChuGL_ShaderInfo wgsl_info;
    wgsl_parse_file(wgsl_path, &wgsl_info);
    chugl_free_shader_info(&wgsl_info);
    return 0;
}
