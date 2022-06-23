#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

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
    }

    printf("\nFinished reading file\n");
}

int decode_box(void *map)
{
    struct BaseBox box;
    memcpy(&box, map, sizeof(box));
    map += sizeof(box);

    // We can't actually read numbers out of the file/map properly due to endian-ness,
    // so we fix it here.
    box.size = htonl(box.size);

    // Next field is optionally largesize, if the first field was set to 1.
    // in practice, I'm not bothering to check for this anywhere else, so it
    // should probably be removed.
    if (box.size == 1)
    {
        printf("Extended box size not supported.\n");
        exit(EXIT_FAILURE);
    }

    // Parse each box type based on the type field we parsed. There's probably a
    // much better way of doing this with some kind of function pointer table.
    if (strncmp(box.type, "ftyp", sizeof(box.type)) == 0)
    {
        decode_ftyp(map, box);
    }
    else if (strncmp(box.type, "mdat", sizeof(box.type)) == 0)
    {
        decode_mdat(map, box);
    }
    else if (strncmp(box.type, "moov", sizeof(box.type)) == 0)
    {
        decode_nested_box(map, box);
    }
    else if (strncmp(box.type, "mvhd", sizeof(box.type)) == 0)
    {
        decode_mvhd(map, box);
    }
    else if (strncmp(box.type, "udta", sizeof(box.type)) == 0)
    {
        decode_nested_box(map, box);
    }
    else if (strncmp(box.type, "trak", sizeof(box.type)) == 0)
    {
        decode_nested_box(map, box);
    }
    else if (strncmp(box.type, "tkhd", sizeof(box.type)) == 0)
    {
        decode_tkhd(map, box);
    }
    else
    {
        printf("%*s[%.4s] size [%d] - unknown box type\n", mp4NestingLevel, "", box.type, box.size);
    }

    // This box continues until the end of the file, so return.
    // We'll decode inner boxes recursively.
    if (box.size == 0)
    {
        return 0;
    }

    return box.size;
}

void decode_nested_box(void *map, const struct BaseBox box)
{
    printf("%*s[%.4s] size [%u]\n", mp4NestingLevel, "", box.type, box.size);

    // this box is nested
    mp4NestingLevel += 2;

    // save the current position in the map and walk ahead until the end of the box
    void *current = map;

    // the end of the box is the current position + the size, minus the size of the
    // size field and the size of the type field, which have already been read
    // in the calling function, and are included in the overall box size
    void *end_of_box = map + box.size - sizeof(box.type) - sizeof(box.size);

    while (current < end_of_box)
    {
        int parsed_bytes = decode_box(current);
        current += parsed_bytes;
    }

    mp4NestingLevel -= 2;
}

void decode_ftyp(void *map, const struct BaseBox box)
{
    // get the major brand & minor version
    struct FtypBox ftypBox;
    memcpy(&ftypBox, (const char *)map, sizeof(ftypBox));
    map += sizeof(ftypBox);

    printf("%*s[ftyp] size [%u] major brand [%.4s] minor version [%.4s] additional brands [", mp4NestingLevel, "", box.size, ftypBox.majorBrand, ftypBox.minorVersion);

    // Additional brands are from here until the end of the box.
    // Have to remove 8 for the size field itself and the type field.
    u_int32_t remainingLength = box.size - 8 - sizeof(ftypBox);

    char additionalBrands[4];
    while (remainingLength > 0)
    {
        memcpy(&additionalBrands, (const char *)map, sizeof(additionalBrands));
        map += sizeof(additionalBrands);
        remainingLength -= sizeof(additionalBrands);
        printf("%.4s,", additionalBrands);
    }
    printf("\b]\n");
}

void decode_mdat(void *map, const struct BaseBox box)
{
    // not much to this box, it's just audio/video data
    printf("%*s[mdat] size [%u]\n", mp4NestingLevel, "", box.size);
}

void decode_mvhd(void *map, const struct BaseBox box)
{
    struct MvhdBox mvhdBox;
    memcpy(&mvhdBox, map, sizeof(mvhdBox));
    map += sizeof(mvhdBox);

    // fix the number endianness
    mvhdBox.creation_time = htonl(mvhdBox.creation_time);
    mvhdBox.mod_time = htonl(mvhdBox.mod_time);
    mvhdBox.timescale = htonl(mvhdBox.timescale);
    mvhdBox.duration = htonl(mvhdBox.duration);
    mvhdBox.rate = htonl(mvhdBox.rate);
    mvhdBox.volume = htons(mvhdBox.volume);

    // there are more fields, but they aren't very interesting

    // couldn't be bothered supporting version 1
    if (mvhdBox.version != 0)
    {
        printf("%*s[mvdh] size [%u] version [%u]\n", mp4NestingLevel, "", box.size, mvhdBox.version);
        return;
    }

    printf("%*s[mvdh] size [%u] version [%u] flags [%#x] creation [%u] modified [%u] timescale [%u] duration [%u] rate [%#x] volume [%#x]\n",
           mp4NestingLevel, "", box.size, mvhdBox.version, mvhdBox.flags, mvhdBox.creation_time, mvhdBox.mod_time, mvhdBox.timescale,
           mvhdBox.duration, mvhdBox.rate, mvhdBox.volume);
}

void decode_tkhd(void *map, const struct BaseBox box)
{
    struct TkhdBox tkhdBox;
    memcpy(&tkhdBox, map, sizeof(tkhdBox));
    map += sizeof(tkhdBox);

    // fix the number endianness
    tkhdBox.version = htonl(tkhdBox.version);
    tkhdBox.creation_time = htonl(tkhdBox.creation_time);
    tkhdBox.mod_time = htonl(tkhdBox.mod_time);
    tkhdBox.track_id = htonl(tkhdBox.track_id);
    tkhdBox.duration = htonl(tkhdBox.duration);
    tkhdBox.width = htonl(tkhdBox.width);
    tkhdBox.height = htonl(tkhdBox.height);

    // couldn't be bothered supporting version 1
    if (tkhdBox.version == 1)
    {
        printf("%*s[tkhd] size [%u] version [%u]\n", mp4NestingLevel, "", box.size, tkhdBox.version);
        return;
    }

    printf("%*s[tkhd] size [%u] version [%u] flags [%#x] creation [%u] modified [%u] track [%u] duration [%u] width [%u] height [%u]\n",
           mp4NestingLevel, "", box.size, tkhdBox.version, tkhdBox.flags, tkhdBox.creation_time, tkhdBox.mod_time, tkhdBox.track_id,
           tkhdBox.duration, tkhdBox.width, tkhdBox.height);
}