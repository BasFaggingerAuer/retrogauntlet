/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//OpenGL video wrapper suitable for libretro cores.
#ifndef GL_VIDEO_H__
#define GL_VIDEO_H__

#include <GL/glew.h>
#include <GL/gl.h>

#include <SDL.h>

#include "retrogauntlet.h"

struct gl_video {
    GLuint base_width, base_height;
    GLuint max_width, max_height;
    GLuint window_width, window_height;
    float aspect_ratio;
    
    GLenum pixel_format;
    GLenum pixel_type;
    GLuint bytes_per_pixel;

    //Framebuffer for libretro.
    GLuint texture;
    GLuint depth_stencil_buffer;
    GLuint frame_buffer;
    
    //OpenGL scene.
    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint vertex_array;
    GLuint vertex_buffer;
    
    struct retro_hw_render_callback core_callback;
};

SDL_Surface *video_get_sdl_surface(const struct gl_video *);
void video_refresh_from_sdl_surface(struct gl_video *, SDL_Surface *);

bool video_set_shaders(struct gl_video *, const char *, const char *);
bool free_video_shaders(struct gl_video *);
bool video_set_geometry(struct gl_video *, const struct retro_game_geometry *);
bool video_set_window(struct gl_video *, const unsigned, const unsigned);
bool video_set_pixel_format(struct gl_video *, const enum retro_pixel_format);
void video_bind_frame_buffer(struct gl_video *);
void video_unbind_frame_buffer(struct gl_video *);
void video_render(const struct gl_video *);
bool free_video_buffers(struct gl_video *);
bool create_video_buffers(struct gl_video *);
bool free_video(struct gl_video *);
bool create_video(struct gl_video *);
void video_refresh_from_libretro(struct gl_video *, const void *, unsigned, unsigned, size_t);

#endif

