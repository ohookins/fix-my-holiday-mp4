#include <stdio.h>
#include <string.h>

#include "mp4.h"

int mp4NestingLevel = 0;

void decode_mp4(const void *map, const int length)
{
    void *current = (void *)map;

    // Need to be able to read the whole box header at least
    // FIXME: Hard-coded literal value
    while (current < map + length)
    {
        // Get the box type and size
        decode_box(current);

        // determine whether we need to recurse into this box or not
        // TODO
    }
}

void decode_box(void *map)
{
    // it's mainly useful to have a structure to store the base metadata in due to
    // the union and optional fields.
    // default to compact size notation, although we'll probably never see extended anyway.
    struct Box box;
    box.size_flag = COMPACT;

    // get the box size and type and delegate further parsing to the parser for the
    // particular box type
    u_int32_t boxsize;
    memcpy(&boxsize, map, sizeof(boxsize));
    map += sizeof(boxsize);

    // get the box type which is in the base box class
    memcpy(&box.type, (const char *)map, sizeof(box.type));
    map += sizeof(box.type);

    // next field is optionally largesize, if the first field was set to 1.
    if (boxsize == 1)
    {
        box.size_flag = EXTENDED;
        memcpy(&box.size.extended, map, sizeof(box.size.extended));
        map += sizeof(box.size.extended);
    }
    else
    {
        box.size.compact = boxsize;
    }

    // Start recursively parsing nested boxes, if the top-level box is allowed to
    // contain other boxes.
    if (strcmp(box.type, "ftyp"))
    {
        decode_ftyp(map);
    }
    else
    {
        printf("Unknown box type: [%s] %lu\n", box.type, sizeof(box.type));
    }

    // This box continues until the end of the file, so return.
    // We'll decode inner boxes recursively.
    if (boxsize == 0)
    {
        return;
    }
}

void decode_ftyp(void *map)
{
    // get the major brand
    char majorBrand[4];
    char minorVersion[4];
    char additionalBrands[4];

    memcpy(&majorBrand, (const char *)map, sizeof(majorBrand));
    map += sizeof(majorBrand);
    memcpy(&minorVersion, (const char *)map, sizeof(minorVersion));
    map += sizeof(minorVersion);
    memcpy(&additionalBrands, (const char *)map + sizeof(majorBrand) + sizeof(minorVersion), sizeof(additionalBrands));
    map += sizeof(additionalBrands);

    printf("%*s[ftyp] major brand [%.4s] minor version [%.4s] additional brands [%.4s]\n", mp4NestingLevel, "", majorBrand, minorVersion, additionalBrands);
}
