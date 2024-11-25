#pragma once

#include "bootparam.h"

typedef struct
{
    unsigned char magic[4];
    unsigned int size;
    unsigned char type;
    unsigned char features;
    unsigned char width;
    unsigned char height;
    unsigned char baseline;
    unsigned char underline;
    unsigned short fragments_offs;
    unsigned int characters_offs;
    unsigned int ligature_offs;
    unsigned int kerning_offs;
    unsigned int cmap_offs;
} __attribute__((packed)) ssfn_font_t;

void print(int x, int y, char *s);

extern bootparam_t *bootp;
