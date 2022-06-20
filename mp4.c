#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "mp4.h"

int mp4NestingLevel = 0;

void decode_mp4(const void *map, const int length)
{
    void *current = (void *)map;

    // Need to be able to read the whole box header at least
    // FIXME: Hard-coded literal value
    while (current < map + length)
    {
        // Decode the base box, and delegate further to more specific box types.
        // Nested boxes are handled by recursing.
        int parsed_bytes = decode_box(current);
        current += parsed_bytes;

        // determine whether we need to recurse into this box or not
        // TODO
    }

    printf("\nFinished reading file\n");
}

int decode_box(void *map)
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

    // We can't actually read numbers out of the file/map properly due to endian-ness,
    // so we fix it here. Yes this is horrible. I am committing many C sins.
    boxsize = htonl(boxsize);

    // get the box type which is in the base box class
    memcpy(&box.type, (const char *)map, sizeof(box.type));
    map += sizeof(box.type);

    // Next field is optionally largesize, if the first field was set to 1.
    // in practice, I'm not bothering to check for this anywhere else, so it
    // should probably be removed.
    if (boxsize == 1)
    {
        box.size_flag = EXTENDED;
        memcpy(&box.size.extended, map, sizeof(box.size.extended));
        box.size.extended = htonll(box.size.extended);
        map += sizeof(box.size.extended);
    }
    else
    {
        box.size.compact = boxsize;
    }

    // Parse each box type based on the type field we parsed. There's probably a
    // much better way of doing this with some kind of function pointer table.
    if (strcmp(box.type, "ftyp") == 0)
    {
        decode_ftyp(map, box);
    }
    else if (strcmp(box.type, "mdat") == 0)
    {
        decode_mdat(map, box);
    }
    else if (strcmp(box.type, "moov") == 0)
    {
        decode_moov(map, box);
    }
    else if (strcmp(box.type, "mvhd") == 0)
    {
        decode_mvhd(map, box);
    }
    else if (strcmp(box.type, "udta") == 0)
    {
        decode_udta(map, box);
    }
    else if (strcmp(box.type, "trak") == 0)
    {
        decode_trak(map, box);
    }
    else
    {
        printf("Unknown box type: [%s]\n", box.type);
    }

    // This box continues until the end of the file, so return.
    // We'll decode inner boxes recursively.
    if (boxsize == 0)
    {
        return 0;
    }

    return boxsize;
}

void decode_ftyp(void *map, const struct Box box)
{
    // get the major brand
    char majorBrand[4];
    char minorVersion[4];
    char additionalBrands[4];

    memcpy(&majorBrand, (const char *)map, sizeof(majorBrand));
    map += sizeof(majorBrand);
    memcpy(&minorVersion, (const char *)map, sizeof(minorVersion));
    map += sizeof(minorVersion);

    printf("%*s[ftyp] size [%u] major brand [%.4s] minor version [%.4s] additional brands [", mp4NestingLevel, "", box.size.compact, majorBrand, minorVersion);

    // Additional brands are from here until the end of the box.
    // Have to remove 8 for the size field itself and the type field.
    u_int32_t remainingLength = box.size.compact - 8 - sizeof(majorBrand) - sizeof(minorVersion);

    while (remainingLength > 0)
    {
        memcpy(&additionalBrands, (const char *)map, sizeof(additionalBrands));
        map += sizeof(additionalBrands);
        remainingLength -= sizeof(additionalBrands);
        printf("%.4s,", additionalBrands);
    }
    printf("\b]\n");
}

void decode_mdat(void *map, const struct Box box)
{
    // not much to this box, it's just audio/video data
    printf("%*s[mdat] size [%u]\n", mp4NestingLevel, "", box.size.compact);
}

void decode_moov(void *map, const struct Box box)
{
    printf("%*s[moov] size [%u]\n", mp4NestingLevel, "", box.size.compact);

    // this box is potentially nested
    mp4NestingLevel += 2;

    void *current = map;
    while (current < map + box.size.compact)
    {
        int parsed_bytes = decode_box(current);
        current += parsed_bytes;
    }

    mp4NestingLevel -= 2;
}

void decode_mvhd(void *map, const struct Box box)
{
    printf("%*s[mvdh] size [%u]\n", mp4NestingLevel, "", box.size.compact);
}

void decode_udta(void *map, const struct Box box)
{
    printf("%*s[udta] size [%u]\n", mp4NestingLevel, "", box.size.compact);
}

void decode_trak(void *map, const struct Box box)
{
    printf("%*s[trak] size [%u]\n", mp4NestingLevel, "", box.size.compact);
}