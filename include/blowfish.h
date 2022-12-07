/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Based directly on the Blowfish code and constants from https://www.schneier.com/academic/blowfish/download/.
#ifndef BLOWFISH_H__
#define BLOWFISH_H__

#include <stdint.h>

struct blowfish {
    uint32_t s[4][256];
    uint32_t p[18];
};

void blowfish_encrypt(const struct blowfish *b, uint32_t * const, uint32_t * const);
void blowfish_decrypt(const struct blowfish *b, uint32_t * const, uint32_t * const);
bool create_blowfish(struct blowfish *, const uint8_t *, const size_t);
bool free_blowfish(struct blowfish *);

#endif

