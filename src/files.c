/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifndef PATH_MAX
#ifdef MAX_PATH
#define PATH_MAX MAX_PATH
#else
#define PATH_MAX 4096
#endif
#endif

char *expand_to_full_path(const char *path) {
    if (!path) return NULL;

#ifdef _WIN32
    char *p = _fullpath(NULL, path, PATH_MAX);
    
    //Replace backslashes by forward slashes.
    if (p) {
        char *q = p;
        while ((q = strchr(q, '\\'))) *q++ = '/';
    }

    return p;
#else
    return realpath(path, NULL);
#endif
}

int does_file_exist(const char *file) {
    if (!file) return 0;

#ifdef _WIN32
    return (_access(file, 0) == 0);
#else
    return (access(file, F_OK) == 0);
#endif
}

char *combine_paths(const char *p1, const char *p2) {
    if (!p1 && !p2) return NULL;
    if (!p1) return strdup(p2);
    if (!p2) return strdup(p1);

    char *p = (char *)calloc(strlen(p1) + strlen(p2) + 2, 1);

    if (!p) return NULL;

    strcpy(p, p1);
    if (strlen(p) > 0) {
        if (p[strlen(p) - 1] != '/' && p[strlen(p) - 1] != '\\') p[strlen(p)] = '/';
    }
    strcpy(p + strlen(p), p2);

    return p;
}

int create_directory(const char *path) {
    if (!path) return 0;
    if (does_file_exist(path)) return 1;
    
#ifdef _WIN32
    return (_mkdir(path) == 0);
#else
    return (mkdir(path, S_IRWXU) == 0);
#endif
}

