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
#include <ctype.h>

#include "stringextra.h"

void strncpy_trim(char *dest, const char *src, const size_t nr) {
    if (!dest) return;

    if (!src) {
        *dest = 0;
        return;
    }
    
    char c;

    for (size_t i = 0; i < nr - 1; i++) {
        if ((c = *src++) == 0) break;
        if (!isspace(c)) *dest++ = c;
    }

    *dest++ = 0;
}

char *read_file_as_string(const char *file) {
    if (!file) return NULL;

    FILE *f = fopen(file, "r");

    if (!f) return NULL;

    fseek(f, 0, SEEK_END);

    const size_t nr_bytes = ftell(f);
    char *string = (char *)calloc(nr_bytes + 1, 1);
    
    rewind(f);
    if (string) {
        size_t nr_read = fread(string, 1, nr_bytes, f);

        if (nr_read != nr_bytes) {
            fprintf(stderr, "read_file_as_string: Read fewer bytes than expected!\n");
        }
    }

    fclose(f);

    return string;
}

char *strcat_and_alloc(const char *s1, const char *s2) {
    if (!s1 && !s2) return NULL;
    if (!s1) return strdup(s2);
    if (!s2) return strdup(s1);

    char *s3 = (char *)calloc(strlen(s1) + strlen(s2) + 1, 1);

    if (!s3) return NULL;
    
    strcpy(s3, s1);
    strcpy(s3 + strlen(s1), s2);

    return s3;
}

