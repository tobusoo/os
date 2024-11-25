#include <stdint.h>

#include "bootparam.h"
#include "print.h"

int _start(bootparam_t *bootpar)
{
    bootp = bootpar;
    for (int i = 0; i < bootp->width * bootp->height; i++)
        bootp->framebuffer[i] = 0x000008;

    print(bootp->height / 2 - 12, bootp->height / 2, "Hello world from kernel!");

    while (1)
    {
    }

    return 0;
}