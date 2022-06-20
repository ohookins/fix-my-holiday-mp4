#include <stdio.h>
#include <string.h>

#include "mp4.h"

void decodeMP4(const void *map, const int length)
{
    // First 4 bytes are the file magic (I guess), haven't yet found the exact
    // reference in the spec for it.
    int relativeLocation = 4;

    __UINT32_TYPE__ boxtype;
    __UINT32_TYPE__ boxsize;

    // Need to be able to read the whole box header at least
    while (relativeLocation + sizeof(boxtype) + sizeof(boxsize) <= length)
    {
        // Get the box type and size
        memcpy(&boxtype, (const char *)map + relativeLocation, sizeof(boxtype));
        relativeLocation += sizeof(boxtype);

        memcpy(&boxsize, (const char *)map + relativeLocation, sizeof(boxsize));
        relativeLocation += sizeof(boxsize);

        printBoxMetadata(boxtype, boxsize);
        return;
    }
}

void printBoxMetadata(const __UINT32_TYPE__ boxtype, const __UINT32_TYPE__ boxsize)
{
    char boxTypeString[5];
    memcpy(boxTypeString, &boxtype, sizeof(boxtype));
    boxTypeString[4] = '\0';

    printf("[%s] box with size: %d\n", boxTypeString, boxsize);
}