/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Base defines and macros for Retro Gauntlet.
#ifndef RETROGAUNTLET_H__
#define RETROGAUNTLET_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "libretro.h"

#define RETRO_GAUNTLET_VERSION "0.0.7"
#define RETRO_GAUNTLET_LOGO "                           RETRO GAUNTLET 0.0.7"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

//https://stackoverflow.com/questions/8776810/parameter-name-omitted-c-vs-c
#if defined(__cplusplus)
#define UNUSED(x)       // = nothing
#elif defined(__GNUC__)
#define UNUSED(x)       x##_UNUSED __attribute__((unused))
#else
#define UNUSED(x)       x##_UNUSED
#endif

#define ERROR_FILE stderr
#define WARN_FILE stderr
#define INFO_FILE stderr
#define CORE_FILE stderr
#define MEM_FILE stdout

#define NR_RETRO_GAUNTLET_PASSWORD 16
#define NR_RETRO_GAUNTLET_NAME 16
#define MAX_RETRO_GAUNTLET_MSG_DATA 65536
#define NR_RETRO_NET_FILE_DATA 32768

#define RETRO_GAUNTLET_NET_HEADER 0xf1b2

#define MAX_RETRO_GAUNTLET_CLIENTS 64

#define IS_COMMENT_LINE(line) (strlen(line) == 0 || line[0] == '#' || line[0] == ';' || line[0] == '/')

#endif

