/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <GL/glu.h>

#include "glcheck.h"

void check_opengl_error(const char *statement, const char *file_name, const int line) {
    const GLenum error = glGetError();
    
    if (error != GL_NO_ERROR) {
        fprintf(ERROR_FILE, "OpenGL error %d (%s) at %s: %d for '%s'!\n", error, gluErrorString(error), file_name, line, statement);
        abort();
    }
}
