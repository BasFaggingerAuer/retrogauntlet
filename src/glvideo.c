/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include "glvideo.h"
#include "glcheck.h"

//Default shaders.
const char *default_vertex_shader_code =
"#version 330\n"
"\n"
"in vec2 vs_tex;\n"
"in vec2 vs_vertex;\n"
"\n"
"out vec2 fs_tex;\n"
"\n"
"void main() {\n"
"   fs_tex = vs_tex;\n"
"   gl_Position = vec4(vs_vertex.xy, 0.0f, 1.0f);\n"
"}\n";

const char *default_fragment_shader_code =
"#version 330\n"
"\n"
"uniform sampler2D framebuffer;\n"
"uniform vec2 framebuffer_size;\n"
"uniform vec2 texture_size;\n"
"uniform vec2 window_size;\n"
"\n"
"in vec2 fs_tex;\n"
"out vec4 frag;\n"
"\n"
"void main() {\n"
"   frag = vec4(texture(framebuffer, fs_tex).xyz, 1.0f);\n"
"}\n";

GLuint gl_compile_shader(const GLchar *code, const GLuint shader_type) {
    GLuint shader = glCreateShader(shader_type);

    GL_CHECK(glShaderSource(shader, 1, &code, NULL));
    GL_CHECK(glCompileShader(shader));

    GLint is_compiled = GL_FALSE;

    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled));

    if (is_compiled != GL_TRUE) {
        GLint nr_log = 0;

        GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &nr_log));

        if (nr_log > 0) {
            GLchar *shader_log = (GLchar *)calloc(nr_log + 1, sizeof(GLchar));

            if (shader_log) {
                GL_CHECK(glGetShaderInfoLog(shader, nr_log, 0, shader_log));
                shader_log[nr_log] = 0;
                fprintf(ERROR_FILE, "gl_compile_shader: Unable to compile shader: %s!\n", shader_log);
                free(shader_log);
            }
        }
        else {
            fprintf(ERROR_FILE, "gl_compile_shader: Unable to compile shader!\n");
        }
    }

    return shader;
}

bool free_video_shaders(struct gl_video *video) {
    if (!video) {
        fprintf(ERROR_FILE, "video_free_shaders: Invalid video!\n");
        return false;
    }
    
    if (video->program) glDeleteProgram(video->program);
    if (video->vertex_shader) glDeleteShader(video->vertex_shader);
    if (video->fragment_shader) glDeleteShader(video->fragment_shader);
    if (video->vertex_array) glDeleteVertexArrays(1, &video->vertex_array);
    if (video->vertex_buffer) glDeleteBuffers(1, &video->vertex_buffer);

    video->program = 0;
    video->vertex_shader = 0;
    video->fragment_shader = 0;
    video->vertex_array = 0;
    video->vertex_buffer = 0;
    
    return true;
}

void video_update_screen_quad(struct gl_video *video) {
    if (!video || !video->vertex_buffer || !video->program) return;

    //Update quad to right dimensions.
    const GLfloat w = (GLfloat)video->base_width/(GLfloat)video->max_width;
    const GLfloat h = (GLfloat)video->base_height/(GLfloat)video->max_height;
    const GLfloat ar = (float)video->window_width/(float)video->window_height;
    const GLfloat dx = min(1.0f, video->aspect_ratio/ar);
    const GLfloat dy = min(1.0f, ar/video->aspect_ratio);
    const GLfloat vertex_data[] = {0.0f, 0.0f, -dx,  dy,
                                   0.0f, h,    -dx, -dy,
                                   w,    0.0f,  dx,  dy,
                                   w,    h,     dx, -dy};

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, video->vertex_buffer));
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, 2*2*4*sizeof(GLfloat), vertex_data));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    
    GL_CHECK(glUseProgram(video->program));
    GL_CHECK(glUniform2f(glGetUniformLocation(video->program, "framebuffer_size"), video->max_width, video->max_height));
    GL_CHECK(glUniform2f(glGetUniformLocation(video->program, "texture_size"), video->base_width, video->base_height));
    GL_CHECK(glUniform2f(glGetUniformLocation(video->program, "window_size"),
        min(video->window_width, (unsigned)((float)video->window_height*video->aspect_ratio)),
        min(video->window_height, (unsigned)((float)video->window_width/video->aspect_ratio))));
    GL_CHECK(glUseProgram(0));
}

bool video_set_shaders(struct gl_video *video, const char *vertex_shader_code, const char *fragment_shader_code) {
    if (!video) {
        fprintf(ERROR_FILE, "video_set_shaders: Invalid video!\n");
        return false;
    }

    free_video_shaders(video);
    
    //Use defaults if none are provided.
    if (!vertex_shader_code) vertex_shader_code = default_vertex_shader_code;
    if (!fragment_shader_code) fragment_shader_code = default_fragment_shader_code;

    //Create shader program.
    video->program = glCreateProgram();
    video->vertex_shader = gl_compile_shader(vertex_shader_code, GL_VERTEX_SHADER);
    video->fragment_shader = gl_compile_shader(fragment_shader_code, GL_FRAGMENT_SHADER);

    if (!video->program || !video->vertex_shader || !video->fragment_shader) {
        fprintf(ERROR_FILE, "video_set_shaders: Unable to create OpenGL program or shaders!\n");
        return false;
    }

    GL_CHECK(glAttachShader(video->program, video->vertex_shader));
    GL_CHECK(glAttachShader(video->program, video->fragment_shader));
    GL_CHECK(glBindAttribLocation(video->program, 0, "vs_tex"));
    GL_CHECK(glBindAttribLocation(video->program, 1, "vs_vertex"));
    GL_CHECK(glLinkProgram(video->program));

    GLint is_program_linked = GL_FALSE;

    GL_CHECK(glGetProgramiv(video->program, GL_LINK_STATUS, &is_program_linked));

    if (is_program_linked != GL_TRUE) {
        fprintf(ERROR_FILE, "video_set_shaders: Unable to link OpenGL program!\n");
        return false;
    }

    //Set shader program's variables.
    GL_CHECK(glUseProgram(video->program));
    GL_CHECK(glUniform1i(glGetUniformLocation(video->program, "framebuffer"), 0));
    GL_CHECK(glUniform2f(glGetUniformLocation(video->program, "framebuffer_size"), video->max_width, video->max_height));
    GL_CHECK(glUniform2f(glGetUniformLocation(video->program, "texture_size"), video->base_width, video->base_height));
    GL_CHECK(glUniform2f(glGetUniformLocation(video->program, "window_size"), video->window_width, video->window_height));
    GL_CHECK(glUseProgram(0));

    //Create vertex buffer for screen-filling quad.
    GL_CHECK(glGenBuffers(1, &video->vertex_buffer));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, video->vertex_buffer));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, 2*2*4*sizeof(GLfloat), NULL, GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL_CHECK(glGenVertexArrays(1, &video->vertex_array));
    GL_CHECK(glBindVertexArray(video->vertex_array));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, video->vertex_buffer));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (const void *)(0*sizeof(GLfloat))));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (const void *)(2*sizeof(GLfloat))));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CHECK(glBindVertexArray(0));

    video_update_screen_quad(video);

    return true;
}

bool video_set_geometry(struct gl_video *video, const struct retro_game_geometry *geometry)
{
    if (!video || !geometry) {
        fprintf(ERROR_FILE, "video_set_geometry: Invalid video or geometry!\n");
        return false;
    }
    
    video->base_width = geometry->base_width;
    video->base_height = geometry->base_height;
    video->max_width = geometry->max_width;
    video->max_height = geometry->max_height;

    if (geometry->aspect_ratio <= 0.0f) {
        video->aspect_ratio = (float)geometry->base_width / (float)geometry->base_height;
    }
    else {
        video->aspect_ratio = geometry->aspect_ratio;
    }
    
    video_update_screen_quad(video);
    
    fprintf(INFO_FILE, "Set screen geometry to (%u, %u), max (%u, %u), aspect ratio %.3f.\n", video->base_width, video->base_height, video->max_width, video->max_height, video->aspect_ratio);

    return true;
}

bool video_set_window(struct gl_video *video, const unsigned width, const unsigned height) {
    if (!video) {
        fprintf(ERROR_FILE, "video_set_window: Invalid video!\n");
        return false;
    }

    video->window_width = width;
    video->window_height = height;
    
    video_update_screen_quad(video);
    
    return true;
}

unsigned get_alignment(unsigned pitch) {
    if (pitch & 1) return 1;
    if (pitch & 2) return 2;
    if (pitch & 4) return 4;
    return 8;
}

void video_render(const struct gl_video *video) {
    if (!video || !video->frame_buffer || !video->texture || !video->program) return;

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_CHECK(glViewport(0, 0, video->window_width, video->window_height));
    GL_CHECK(glScissor(0, 0, video->window_width, video->window_height));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, video->texture));
    GL_CHECK(glUseProgram(video->program));
    GL_CHECK(glBindVertexArray(video->vertex_array));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glUseProgram(0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
}

/*
void video_draw_to_screen(struct gl_video *video, unsigned dest_x0, unsigned dest_y0, unsigned dest_x1, unsigned dest_y1) {
    if (!video || !video->frame_buffer || !video->texture) return;
    
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, video->texture));
    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, video->frame_buffer));
    GL_CHECK(glReadBuffer(GL_COLOR_ATTACHMENT0));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));

    if (!video->core_callback.bottom_left_origin) {
        GL_CHECK(glBlitFramebuffer(0, video->base_height, video->base_width, 0,
                                   dest_x0, dest_y0, dest_x1, dest_y1,
                                   GL_COLOR_BUFFER_BIT, GL_NEAREST));
    }
    else {
        GL_CHECK(glBlitFramebuffer(0, 0, video->base_width, video->base_height,
                                   dest_x0, dest_y0, dest_x1, dest_y1,
                                   GL_COLOR_BUFFER_BIT, GL_NEAREST));
    }

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
}
*/

SDL_Surface *video_get_sdl_surface(const struct gl_video *video) {
    if (!video) {
        fprintf(ERROR_FILE, "video_get_sdl_surface: Invalid video!\n");
        return NULL;
    }
    
    //Setup masks depending on endianness.
    Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    SDL_Surface *surf = SDL_CreateRGBSurface(0, video->base_width, video->base_height, 32, rmask, gmask, bmask, amask);

    if (!surf) {
        fprintf(ERROR_FILE, "Unable to create SDL surface: %s!\n", SDL_GetError());
        return NULL;
    }

    return surf;
}

void video_refresh_from_sdl_surface(struct gl_video *video, SDL_Surface *surf) {
    if (!surf || !video) return;

    if ((int)video->base_width != surf->w || (int)video->base_height != surf->h) {
        video->base_width = surf->w;
        video->base_height = surf->h;
        video_update_screen_quad(video);
    }
    
    SDL_LockSurface(surf);
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, video->texture));
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(surf->pitch)));
    GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, surf->pitch/surf->format->BytesPerPixel));
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, video->pixel_format, video->pixel_type, surf->pixels));
    GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    SDL_UnlockSurface(surf);
}

void video_refresh_from_libretro(struct gl_video *video, const void *data, unsigned width, unsigned height, size_t pitch) {
    if (!data || !video) return;

    if (video->base_width != width || video->base_height != height) {
        video->base_width = width;
        video->base_height = height;
        video_update_screen_quad(video);
    }

    if (data && data != RETRO_HW_FRAME_BUFFER_VALID) {
        /*
        printf("BUFFER %u, %u to texture %u\n", width, height, video->texture);
        
        uint32_t test[] = {0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff,
                           0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff,
                           0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff,
                           0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff,
                           0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff,
                           0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff,
                           0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff, 0xffffffff, 0xff0000ff};

        GL_CHECK(glBindTexture(GL_TEXTURE_2D, video->texture));
        GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(8*4)));
        GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 8));
        GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 8, 7, video->pixel_format, video->pixel_type, test));
        GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
        */

        GL_CHECK(glBindTexture(GL_TEXTURE_2D, video->texture));
        GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch)));
        GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch/video->bytes_per_pixel));
        GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, video->pixel_format, video->pixel_type, data));
        GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    }
}

bool video_set_pixel_format(struct gl_video *video, const enum retro_pixel_format format) {
    if (!video) {
        fprintf(ERROR_FILE, "video_set_pixel_format: Invalid video!\n");
        return false;
    }
    
    switch (format) {
        case RETRO_PIXEL_FORMAT_0RGB1555:
            video->pixel_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            video->pixel_format = GL_BGRA;
            video->bytes_per_pixel = 2;
            break;
        case RETRO_PIXEL_FORMAT_XRGB8888:
            video->pixel_type = GL_UNSIGNED_INT_8_8_8_8_REV;
            video->pixel_format = GL_BGRA;
            video->bytes_per_pixel = 4;
            break;
        case RETRO_PIXEL_FORMAT_RGB565:
            video->pixel_type = GL_UNSIGNED_SHORT_5_6_5;
            video->pixel_format = GL_RGB;
            video->bytes_per_pixel = 2;
            break;
        case RETRO_PIXEL_FORMAT_GAUNTLET:
            video->pixel_type = GL_UNSIGNED_BYTE;
            video->pixel_format = GL_RGBA;
            video->bytes_per_pixel = 4;
            break;
        case RETRO_PIXEL_FORMAT_UNKNOWN:
        default:
            fprintf(ERROR_FILE, "video_set_pixel_format: Unknown pixel format %u!\n", format);
            return false;
    }

    fprintf(INFO_FILE, "Set screen pixel format to %u, %u, %u bytes per pixel from %u.\n", video->pixel_type, video->pixel_format, video->bytes_per_pixel, format);

    return true;
}

void video_bind_frame_buffer(struct gl_video *video) {
    if (!video) {
        fprintf(ERROR_FILE, "video_bind_frame_buffer: Invalid video!\n");
        return;
    }
    
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, video->frame_buffer));
    GL_CHECK(glViewport(0, 0, video->base_width, video->base_height));
    GL_CHECK(glScissor(0, 0, video->base_width, video->base_height));
}

void video_unbind_frame_buffer(struct gl_video *video) {
    if (!video) {
        fprintf(ERROR_FILE, "video_bind_frame_buffer: Invalid video!\n");
        return;
    }
    
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
}

bool free_video_buffers(struct gl_video *video) {
    //Will only free allocated data, size settings will be retained.
    if (!video) {
        fprintf(ERROR_FILE, "free_video_buffers: Invalid video object!\n");
        return false;
    }

    if (video->frame_buffer) glDeleteFramebuffers(1, &video->frame_buffer);
    video->frame_buffer = 0;

    if (video->depth_stencil_buffer) glDeleteRenderbuffers(1, &video->depth_stencil_buffer);
    video->depth_stencil_buffer = 0;

    if (video->texture) glDeleteTextures(1, &video->texture);
    video->texture = 0;

    return true;
}

bool create_video_buffers(struct gl_video *video) {
    if (!video || video->base_width == 0 || video->base_height == 0 || video->base_width > video->max_width || video->base_height > video->max_height) {
        fprintf(ERROR_FILE, "create_video_buffers: Invalid video object or invalid screen dimensions!\n");
        return false;
    }
    
    //Free all existing buffers.
    free_video_buffers(video);
    
    //Create texture.
    GL_CHECK(glGenTextures(1, &video->texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, video->texture));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, video->max_width, video->max_height, 0, video->pixel_format, video->pixel_type, NULL));

    //Create framebuffer.
    GL_CHECK(glGenFramebuffers(1, &video->frame_buffer));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, video->frame_buffer));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, video->texture, 0));

    //Create optional depth buffer.
    if (video->core_callback.depth) {
        GL_CHECK(glGenRenderbuffers(1, &video->depth_stencil_buffer));
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, video->depth_stencil_buffer));
        GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, video->core_callback.stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24, video->max_width, video->max_height));
        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, video->core_callback.stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, video->depth_stencil_buffer));
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
    }
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(ERROR_FILE, "create_video_buffers: Incomplete framebuffer!\n");
        return false;
    }

    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    video_bind_frame_buffer(video);
    if (video->core_callback.context_reset) video->core_callback.context_reset();
    video_unbind_frame_buffer(video);
    
    video_update_screen_quad(video);

    fprintf(INFO_FILE, "Created OpenGL framebuffer and associated data for (%u, %u), max (%u, %u), aspect ratio %.3f.\n", video->base_width, video->base_height, video->max_width, video->max_height, video->aspect_ratio);

    return true;
}

bool free_video(struct gl_video *video) {
    if (!video) {
        fprintf(ERROR_FILE, "free_video: Invalid video object!\n");
        return false;
    }

    //g_video.hw.context_destroy();?
    free_video_buffers(video);
    free_video_shaders(video);
    memset(video, 0, sizeof(struct gl_video));

    return true;
}

bool create_video(struct gl_video *video) {
    if (!video) {
        fprintf(ERROR_FILE, "create_video: Invalid video object!\n");
        return false;
    }
    
    memset(video, 0, sizeof(struct gl_video));

    //Setup with default settings.
    video->base_width = 640;
    video->base_height = 480;
    video->max_width = video->base_width;
    video->max_height = video->base_height;
    video->window_width = video->base_width;
    video->window_height = video->base_height;
    video->aspect_ratio = (float)video->base_width/(float)video->base_height;
    
    //Default per libretro.h.
    video->pixel_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
    video->pixel_format = GL_BGRA;
    video->bytes_per_pixel = 2;

    return true;
}


