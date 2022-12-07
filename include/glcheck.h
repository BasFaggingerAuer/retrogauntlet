/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Verify that OpenGL commands are issued correctly.
#ifndef GL_CHECK_H__
#define GL_CHECK_H__

#include "retrogauntlet.h"

//From http://stackoverflow.com/questions/11256470/define-a-macro-to-facilitate-opengl-command-debugging.
void check_opengl_error(const char *statement, const char *file_name, const int line);

#ifndef NDEBUG
    #define GL_CHECK(statement) do { \
            statement; \
            check_opengl_error(#statement, __FILE__, __LINE__); \
        } while (0)
#else
    #define GL_CHECK(stmt) stmt
#endif

#endif

