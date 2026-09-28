
#ifndef CHUGL_WGSL_REFLECT_H
#define CHUGL_WGSL_REFLECT_H

#define CHUGL_MAX_INCLUDES 16
#define CHUGL_MAX_UNIFORMS 32

typedef enum ChuGL_UniformType {
    CHUGL_F32,
    CHUGL_VEC2F,
    CHUGL_VEC3F,
    CHUGL_VEC4F,
    CHUGL_I32,
    CHUGL_VEC2I,
    CHUGL_VEC3I,
    CHUGL_VEC4I,
    CHUGL_TEXTURE_2D,
    CHUGL_SAMPLER,
    CHUGL_TEXTURE_STORAGE_2D,
    CHUGL_STORAGE_BUFFER,
    CHUGL_UNSUPPORTED
} ChuGL_UniformType;

typedef struct ChuGL_Uniform {
    int               group;
    int               binding;
    ChuGL_UniformType type;
    char*             name;
} ChuGL_Uniform;

typedef struct ChuGL_ShaderInfo {
    char*         includes[CHUGL_MAX_INCLUDES];
    ChuGL_Uniform uniforms[CHUGL_MAX_UNIFORMS];
    int           include_count;
    int           uniform_count;
} ChuGL_ShaderInfo;

void wgsl_parse_string(char* wgsl_string, ChuGL_ShaderInfo* out);

void wgsl_parse_file(const char* wgsl_path, ChuGL_ShaderInfo* out);

void chugl_free_shader_info(ChuGL_ShaderInfo* info);

#endif /* CHUGL_WGSL_REFLECT_H */

#ifdef CHUGL_WGSL_REFLECT_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

#define CHUGL_WHITESPACE_CHARS " \t\n\r\v\f"
#define CHUGL_INCLUDE "#include"
#define CHUGL_GROUP_PREFIX "@group("
#define CHUGL_BINDING_PREFIX "@binding("

#define CHUGL_CHECK_NULL(ptr, msg) \
    if(!(ptr)) {\
        fprintf(stderr, (msg));\
        return NULL;\
    }

static char *read_file(const char* path) {
    FILE *file = fopen(path, "rb");

    if (!file) {
        perror("Failed to open file");
        fclose(file);
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Error seeking file end");
        fclose(file);
        return NULL;
    }
    size_t file_size = ftell(file);
    if (file_size < 0) {
        perror("Error reading file pointer");
        fclose(file);
        return NULL;
    }

    rewind(file);

    char* buff = (char*)malloc(file_size + 1);
    if (!buff) {
        perror("Failed to allocate buffer");
        fclose(file);
        return NULL;
    }
    size_t buff_size = fread(buff, 1, file_size, file);
    if (buff_size < file_size) {
        ferror(file);  // Set errno
        perror("Could not read entire file");
        fclose(file);
        free(buff);
        return NULL;
    }
    buff[buff_size] = '\0';

    fclose(file);
    return buff;
}

static const char* seek_whitespace(const char* cursor) {
    while(*cursor && !strchr(CHUGL_WHITESPACE_CHARS, *cursor)) {
        cursor++;
    }
    return cursor;
}

static const char* skip_whitespace(const char* cursor) {
    while(*cursor && strchr(CHUGL_WHITESPACE_CHARS, *cursor)) {
        cursor++;
    }
    return cursor;
}

/* Checks if start points at the beginning of a token, whitespace delimited.
   Does not modify the input string. */
static int is_token(const char* start, const char* token) {
    const char* end = seek_whitespace(start);
    if(end - start != strlen(token)) return 0;
    return strncmp(start, token, strlen(token)) == 0;
}

/* Replaces comments with whitespace in-place.
   In the case the file ends with a comment, the string is terminated early. */
static void strip_comments(char* wgsl_string) {
    // Single-line comments
    char* cursor = strstr(wgsl_string, "//");
    while(cursor) {
        char* comment_end = strchr(cursor, '\n');  // End at line break
        if (!comment_end) {  // Last line of the file
            *cursor = '\0';
            break;
        }
        memset(cursor,  ' ', comment_end - cursor);  // Replace with whitespace
        cursor = strstr(wgsl_string, "//");
    }

    // Multi-line comments
    cursor = strstr(wgsl_string, "/*");
    while(cursor) {
        char* comment_end = strstr(cursor, "*/");
        if (!comment_end) {
            // File is still parsable but won't compile
            fprintf(stderr, "Unterminated comment: %.20s...\n", cursor);
            *cursor = '\0';
            break;
        }
        memset(cursor, ' ', comment_end - cursor + 2);
        cursor = strstr(wgsl_string, "/*"); 
    }
}

/* Populates u with the results of parsing the uniform.
   Returns a pointer to the char after the uniform, or NULL on error. */
static const char* parse_uniform(const char* uniform_start, ChuGL_Uniform* u) {
    const char *uniform_end = strchr(uniform_start, ';');
    const char *uniform_decl_start = strstr(uniform_start, "var");
    const char *uniform_group_start   = strstr(uniform_start, CHUGL_GROUP_PREFIX);
    const char *uniform_binding_start = strstr(uniform_start, CHUGL_BINDING_PREFIX);

    CHUGL_CHECK_NULL(uniform_end, "Uniform declaration missing semicolon\n")
    CHUGL_CHECK_NULL(uniform_decl_start, "Uniform declaration missing scope\n")
    CHUGL_CHECK_NULL(uniform_group_start, "Uniform declaration missing group\n")
    CHUGL_CHECK_NULL(uniform_binding_start, "Uniform declaratioin missing binding\n")

    const char* uniform_group_num = uniform_group_start + strlen(CHUGL_GROUP_PREFIX);
    const char* uniform_binding_num = uniform_binding_start + strlen(CHUGL_BINDING_PREFIX);

    const char *uniform_name_start = uniform_decl_start + 3;
    if (strncmp(uniform_decl_start, "var<", 4) == 0) {
        uniform_name_start = strchr(uniform_decl_start, '>');
        CHUGL_CHECK_NULL(uniform_name_start, "Uniform scope declaration is invalid\n")
        uniform_name_start++;  // Move past the end bracket
    }
    uniform_name_start = skip_whitespace(uniform_name_start);
    
    // Note: name and type not properly handling whitespace, don't forget to implement
    const char* uniform_name_end = strchr(uniform_name_start, ':');
    CHUGL_CHECK_NULL(uniform_name_end, "Uniform type missing\n");
    const char* uniform_type_start = skip_whitespace(++uniform_name_end);

    // Parse type
    if (strncmp(uniform_type_start, "f32", 3) == 0) { u->type = CHUGL_F32; } 
    else { u->type = CHUGL_UNSUPPORTED; }

    // Allocate and set name
    int name_len = uniform_name_end - uniform_name_start - 1;  // Exclude ':'
    char* uniform_name = (char*)malloc(name_len + 1);
    memcpy(uniform_name, uniform_name_start, name_len);
    uniform_name[name_len] = '\0';
    u->name = uniform_name;

    char* end;
    u->group = (int)strtol(uniform_group_num , &end, 0);
    if (uniform_group_num == end || *end != ')') {
        fprintf(stderr, "Invalid bind group number\n");
        //return NULL;
    }
    u->binding = (int)strtol(uniform_binding_num, &end, 0);
    if (uniform_binding_num == end || *end != ')') {
        fprintf(stderr, "Invalid binding number\n");
        //return NULL;
    }
    return uniform_end + 1;
}

void wgsl_parse_string(char* wgsl_string, ChuGL_ShaderInfo* out) {
    // Replace comments with whitespace in-place
    strip_comments(wgsl_string);
    
    // Collect includes
    int inc_idx = 0;
    const char* hash = strchr(wgsl_string, '#');
    while(hash) {
        if(is_token(hash, CHUGL_INCLUDE)) {
            if(inc_idx >= CHUGL_MAX_INCLUDES) {
                fprintf(stderr, "Include limit of %d exceeded", 
                        CHUGL_MAX_INCLUDES);
                break;
            }
            const char* inc_start = skip_whitespace(hash + strlen(CHUGL_INCLUDE));
            const char* inc_end   = seek_whitespace(inc_start);
            int   inc_len   = inc_end - inc_start;
            char* inc_str   = (char*)malloc(inc_len + 1);
            if (!inc_str) {
                fprintf(stderr, "Malloc failed");
                return;
            }
            memcpy(inc_str, inc_start, inc_len);
            inc_str[inc_len] = '\0';
            out->includes[inc_idx++] = inc_str;
            hash = inc_end;
        }
        hash = strchr(hash, '#');
    }
    out->include_count = inc_idx;

    for(int i = 0; i < out->include_count; i++) {
        printf("#include %s\n", out->includes[i]);
    }

    // Collect uniforms
    int uni_idx = 0;
    const char* elem = strstr(wgsl_string, CHUGL_GROUP_PREFIX);
    while(elem) {
        ChuGL_Uniform u;
        elem = parse_uniform(elem, &u);
        elem = strstr(elem, CHUGL_GROUP_PREFIX);

        if(uni_idx >= CHUGL_MAX_UNIFORMS) {
            fprintf(stderr, "Uniform limit of %d exceeded", 
                    CHUGL_MAX_UNIFORMS);
            break;
        }

        out->uniforms[uni_idx] = u;
        uni_idx++;
    }
    out->uniform_count = uni_idx;

    for(int i = 0; i < out->uniform_count; i++) {
        ChuGL_Uniform* u = &(out->uniforms[i]);
        printf("{name:\"%s\", type:%d, group:%d, binding:%d}\n",
               u->name, u->type, u->group, u->binding);
    }
}

void wgsl_parse_file(const char* wgsl_path, ChuGL_ShaderInfo* out) {
    char* wgsl_file = read_file(wgsl_path);

    if (wgsl_file == NULL) {  // Error reading input file
        out = NULL;
        return;
    }

    wgsl_parse_string(wgsl_file, out);

    free(wgsl_file);
}

void chugl_free_shader_info(ChuGL_ShaderInfo* info) {
    for (int i = 0; i < info->include_count; i++) {
        free(info->includes[i]);
    }
    for (int i = 0; i < info->uniform_count; i++) {
        free(info->uniforms[i].name);
    }
}

#endif /* CHUGL_WGSL_REFLECT_IMPLEMENTATION */
