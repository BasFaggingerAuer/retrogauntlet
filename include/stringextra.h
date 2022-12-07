/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Some basic functions that should really be built-ins for C.
#ifndef _STRING_EXTRA_H__
#define _STRING_EXTRA_H__

void strncpy_trim(char *dest, const char *src, const size_t nr);
char *read_file_as_string(const char *file);
char *strcat_and_alloc(const char *s1, const char *s2);

#endif

