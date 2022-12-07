/*
Copyright 2022 Bas Fagginger Auer.
This file is part of Retro Gauntlet.

Retro Gauntlet is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Retro Gauntlet is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Retro Gauntlet. If not, see <https://www.gnu.org/licenses/>.
*/
//Based directly on the Blowfish code and constants from https://www.schneier.com/academic/blowfish/download/.
#include <stdio.h>
#include <stdint.h>

#include "retrogauntlet.h"

#include "blowfishhexpi.h"
#include "blowfish.h"

uint32_t F(const struct blowfish *b, uint32_t x) {
    uint32_t y = b->s[0][(x >> 0x18) & 0xff] + b->s[1][(x >> 0x10) & 0xff];

    y ^= b->s[2][(x >> 0x08) & 0xff];
    y += b->s[3][x & 0xff];

    return y;
}

void blowfish_encrypt(const struct blowfish *b, uint32_t * const lp, uint32_t * const rp) {
    uint32_t l = *lp, r = *rp, t;
    
    for (short i = 0; i < 16; ++i) {
        l ^= b->p[i];
        r ^= F(b, l);
        t = l; l = r; r = t;
    }

    t = l; l = r; r = t;
    r ^= b->p[16];
    l ^= b->p[17];

    *lp = l;
    *rp = r;
}

void blowfish_decrypt(const struct blowfish *b, uint32_t * const lp, uint32_t * const rp) {
    uint32_t l = *lp, r = *rp, t;
    
    for (short i = 17; i > 1; --i) {
        l ^= b->p[i];
        r ^= F(b, l);
        t = l; l = r; r = t;
    }

    t = l; l = r; r = t;
    r ^= b->p[1];
    l ^= b->p[0];

    *lp = l;
    *rp = r;
}

bool create_blowfish(struct blowfish *b, const uint8_t *key, const size_t nr_key) {
    if (!b || !key) {
        fprintf(ERROR_FILE, "create_blowfish: Invalid blowfish or key!\n");
        return false;
    }
    
    //Initialize arrays with digits of pi.
    //FIXME: Will this work if machines have different endianness?
    memset(b, 0, sizeof(struct blowfish));
    memcpy(b->s[0], blowfish_sbox0, 256*sizeof(uint32_t));
    memcpy(b->s[1], blowfish_sbox1, 256*sizeof(uint32_t));
    memcpy(b->s[2], blowfish_sbox2, 256*sizeof(uint32_t));
    memcpy(b->s[3], blowfish_sbox3, 256*sizeof(uint32_t));
    memcpy(b->p, blowfish_p, 18*sizeof(uint32_t));

    //Add key.
    uint32_t data;
    size_t k = 0;

    for (short i = 0; i < 18; ++i) {
        data = 0;

        for (short j = 0; j < 4; ++j) {
            data = (data << 8) | key[k];
            if (++k >= nr_key) k = 0;
        }

        b->p[i] ^= data;
    }
    
    //Expand key.
    uint32_t l = 0, r = 0;

    for (short i = 0; i < 18; i += 2) {
        blowfish_encrypt(b, &l, &r);
        b->p[i] = l;
        b->p[i + 1] = r;
    }

    for (short i = 0; i < 4; ++i) {
        for (short j = 0; j < 256; j += 2) {
            blowfish_encrypt(b, &l, &r);
            b->s[i][j] = l;
            b->s[i][j + 1] = r;
        }
    }

    return true;
}

bool free_blowfish(struct blowfish *b) {
    if (!b) {
        fprintf(ERROR_FILE, "free_blowfish: Invalid blowfish!\n");
        return false;
    }
    
    return true;
}

