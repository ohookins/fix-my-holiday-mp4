#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>

#include "mp4.h"

int mp4NestingLevel = 0;

// oh lord, forgive me for this global variable
u_int32_t timescale;

// homebrewed function pointer
struct PointerTableEntry *function_pointer_table;
const int num_box_types = 10;

void *get_decode_function_for_box_type(const struct BaseBox box)
{
    for (int i = 0; i < num_box_types; i++)
    {
        if (strncmp(box.type, function_pointer_table[i].name, sizeof(box.type)) == 0)
        {
            return function_pointer_table[i].function;
        }
    }

    return unknown_box_type;
}

void decode_mp4(const void *map, const int length)
{
    // construct the function pointer table
    function_pointer_table = malloc(sizeof(struct PointerTableEntry) * num_box_types);
    function_pointer_table[0] = (struct PointerTableEntry){"ftyp", &decode_ftyp};
    function_pointer_table[1] = (struct PointerTableEntry){"mdat", &decode_mdat};
    function_pointer_table[2] = (struct PointerTableEntry){"moov", &decode_nested_box};
    function_pointer_table[3] = (struct PointerTableEntry){"mvhd", &decode_mvhd};
    function_pointer_table[4] = (struct PointerTableEntry){"udta", &decode_nested_box};
    function_pointer_table[5] = (struct PointerTableEntry){"trak", &decode_nested_box};
    function_pointer_table[6] = (struct PointerTableEntry){"mdia", &decode_nested_box};
    function_pointer_table[7] = (struct PointerTableEntry){"tkhd", &decode_tkhd};
    function_pointer_table[8] = (struct PointerTableEntry){"mdhd", &decode_mdhd};
    function_pointer_table[9] = (struct PointerTableEntry){"hdlr", &decode_hdlr};

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

    // Delegate further parsing of boxes to the relevant function
    void (*decode_function)(void *, const struct BaseBox) = get_decode_function_for_box_type(box);
    (*decode_function)(map, box);

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
    mvhdBox.timescale = htonl(mvhdBox.timescale); // units per second
    mvhdBox.duration = htonl(mvhdBox.duration);
    mvhdBox.rate = htonl(mvhdBox.rate);
    mvhdBox.volume = htons(mvhdBox.volume);

    // I'm lazy, so set a global for this as it's handy to decode other boxes
    timescale = mvhdBox.timescale;

    // there are more fields, but they aren't very interesting

    // couldn't be bothered supporting version 1
    if (mvhdBox.version != 0)
    {
        printf("%*s[mvdh] size [%u] version [%u]\n", mp4NestingLevel, "", box.size, mvhdBox.version);
        return;
    }

    printf("%*s[mvdh] size [%u] version [%u] flags [%#x] creation [%.24s] modified [%.24s] timescale [%u/sec] duration [%fs] rate [%#x] volume [%#x]\n",
           mp4NestingLevel, "", box.size, mvhdBox.version, mvhdBox.flags, translate_timestamp(mvhdBox.creation_time),
           translate_timestamp(mvhdBox.mod_time), mvhdBox.timescale, (double)mvhdBox.duration / mvhdBox.timescale, mvhdBox.rate, mvhdBox.volume);
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

    // MP4 docs are unclear on this, but one page mentions that width and height
    // are "fixed-point 16.16" fields. The high bytes seemed to contain something
    // reasonable, and the low bytes were all zero, so shift them.
    tkhdBox.width = htonl(tkhdBox.width) >> 16;
    tkhdBox.height = htonl(tkhdBox.height) >> 16;

    // couldn't be bothered supporting version 1
    if (tkhdBox.version == 1)
    {
        printf("%*s[tkhd] size [%u] version [%u]\n", mp4NestingLevel, "", box.size, tkhdBox.version);
        return;
    }

    printf("%*s[tkhd] size [%u] version [%u] flags [%#x] creation [%.24s] modified [%.24s] track [%u] duration [%fs] width [%u] height [%u]\n",
           mp4NestingLevel, "", box.size, tkhdBox.version, tkhdBox.flags, translate_timestamp(tkhdBox.creation_time),
           translate_timestamp(tkhdBox.mod_time), tkhdBox.track_id, (double)tkhdBox.duration / timescale, tkhdBox.width, tkhdBox.height);
}

void decode_mdhd(void *map, const struct BaseBox box)
{
    struct MdhdBox mdhdBox;
    memcpy(&mdhdBox, map, sizeof(mdhdBox));

    // fix the number endianness
    mdhdBox.version = htonl(mdhdBox.version);
    mdhdBox.creation_time = htonl(mdhdBox.creation_time);
    mdhdBox.mod_time = htonl(mdhdBox.mod_time);
    mdhdBox.timescale = htonl(mdhdBox.timescale);
    mdhdBox.duration = htonl(mdhdBox.duration);

    // not even going to check for version 1 here

    printf("%*s[mdhd] size [%u] version [%u] flags [%#x] creation [%.24s] modified [%.24s] timescale [%u/sec] duration [%fs]\n",
           mp4NestingLevel, "", box.size, mdhdBox.version, mdhdBox.flags, translate_timestamp(mdhdBox.creation_time),
           translate_timestamp(mdhdBox.mod_time), mdhdBox.timescale, (double)mdhdBox.duration / mdhdBox.timescale);
}

void decode_hdlr(void *map, const struct BaseBox box)
{
    struct HdlrBox hdlrBox;
    memcpy(&hdlrBox, map, sizeof(hdlrBox));

    // Read the name field
    size_t name_len = box.size - sizeof(hdlrBox);
    char name[name_len];
    memcpy(&name, map + sizeof(hdlrBox), sizeof(name));

    // fix the number endianness
    hdlrBox.version = htonl(hdlrBox.version);

    printf("%*s[hdlr] size [%u] version [%u] flags [%#x] handler type [%s] name [%s]\n",
           mp4NestingLevel, "", box.size, hdlrBox.version, hdlrBox.flags, hdlrBox.handler_type, name);
}

// yes, I'm exceedingly lazy
const u_int32_t seconds_1904_to_1970 = 2082844800;

char *translate_timestamp(const u_int32_t timestamp)
{
    time_t time = (time_t)timestamp - seconds_1904_to_1970;
    return ctime(&time);
}

void unknown_box_type(void *map, const struct BaseBox box)
{
    printf("%*s[%.4s] size [%d] - unknown box type\n", mp4NestingLevel, "", box.type, box.size);
}