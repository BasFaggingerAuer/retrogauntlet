/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <GL/glew.h>
#include <GL/gl.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <SDL_mixer.h>

#include "retrogauntlet.h"

int main(int argc, char **argv) {
    if (argc != 2 && argc != 1) {
        fprintf(ERROR_FILE, "Usage: %s data/\n", argv[0]);
        return EXIT_FAILURE;
    }

    fprintf(INFO_FILE, "Welcome to Retro Gauntlet version %s.\n", RETRO_GAUNTLET_VERSION);
    
    const char *data_directory = ".";

    if (argc > 1) data_directory = argv[1];
    
    //Window opening dimensions.
    unsigned width =  1440;
    unsigned height =  900;

    //Initialize SDL.
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(ERROR_FILE, "Unable to initialize SDL: %s!\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG) < 0) {
        fprintf(ERROR_FILE, "Unable to initialize SDL_mixer: %s!\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    //Set SDL OpenGL video settings.
    SDL_GL_LoadLibrary(NULL);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    //Create window.
    SDL_Window *window = SDL_CreateWindow("Retro Gauntlet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    //Initialize GLEW
    glewExperimental = GL_TRUE;

    const GLenum glew_init_result = glewInit();
    
    if (glew_init_result != GLEW_OK) {
        fprintf(ERROR_FILE, "Unable to initialize GLEW: %s!\n", glewGetErrorString(glew_init_result));
        return EXIT_FAILURE;
    }

    //Create retrogauntlet instance.
    if (!retrogauntlet_initialize(data_directory, window)) {
        return EXIT_FAILURE;
    }
    
    //Main loop.
    bool keep_running = true;
    bool fullscreen = false;
    
    srand(SDL_GetTicks());

    while (keep_running) {
        SDL_Event event;
        
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    keep_running = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE:
                            keep_running = false;
                            break;
                    }
                    break;
            }

            retrogauntlet_sdl_event(event);
        }
        
        if (!retrogauntlet_keep_running()) keep_running = false;

        if (fullscreen != retrogauntlet_fullscreen()) {
            fullscreen = retrogauntlet_fullscreen();
            SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }

        retrogauntlet_frame_update();
    }
    
    //Stop retrogauntlet.
    retrogauntlet_quit();

    //Free SDL data.
    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(context);
    SDL_GL_UnloadLibrary();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();

    fprintf(INFO_FILE, "Shut down Retro Gauntlet version %s.\n", RETRO_GAUNTLET_VERSION);

    return EXIT_SUCCESS;
}

