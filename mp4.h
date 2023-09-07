#include <stdbool.h>

// Struct definitions for the box types I'm interested in.
// I'm not bothering with extended size boxes or additional fields, to keep
// things simple. I'm also not defining any box types that aren't useful for the
// media files I have on hand.
struct BaseBox
{
    u_int32_t size;
    char type[4];
};

// For making a function pointer table. There are probably better ways to do this.
struct PointerTableEntry
{
    char name[4];
    void (*function)(void *, const struct BaseBox);
};

// ftyp always has a variable length, these are the only two guaranteed fields
struct FtypBox
{
    char majorBrand[4];
    char minorVersion[4];
};

struct MvhdBox
{
    u_int8_t version;
    unsigned int flags : 24;
    u_int32_t creation_time;
    u_int32_t mod_time;
    u_int32_t timescale;
    u_int32_t duration;
    int32_t rate;
    int16_t volume;
};

struct TkhdBox
{
    u_int8_t version;
    unsigned int flags : 24;
    u_int32_t creation_time;
    u_int32_t mod_time;
    u_int32_t track_id;
    u_int32_t reserved1;
    u_int32_t duration;
    int32_t reserved2[2];
    int16_t layer;
    int16_t alternate_group;
    int16_t volume;
    u_int16_t reserved3;
    int32_t unity_matrix[9];
    u_int32_t width;
    u_int32_t height;
};

struct MdhdBox
{
    u_int8_t version;
    unsigned int flags : 24;
    u_int32_t creation_time;
    u_int32_t mod_time;
    u_int32_t timescale;
    u_int32_t duration;
    unsigned int padding : 1;

    // declares the language code for this media. See ISO 639-2/T for the set of three character
    // codes. Each character is packed as the difference between its ASCII value and 0x60. Since the code
    // is confined to being three lower-case letters, these values are strictly positive.
    // unsigned int(5)[3] language; // ISO-639-2/T language code
    // I can't figure out how to make the above specification work in C so I'm leaving it for now.
    unsigned int languages : 15;
    u_int16_t pre_defined;
};

struct HdlrBox
{
    u_int8_t version;
    unsigned int flags : 24;
    u_int32_t pre_defined;
    char handler_type[4]; // u_int32_t in the specification but it's actually a four-character handler type code
    u_int32_t reserved[3];

    // name is a null-terminated string of variable length, so it's not useful
    // to have as a pointer in this struct
    // char *name;
};

void *
get_decode_function_for_box_type(const struct BaseBox box);

// decode_mp4 starts the overall process of decoding the MP4 file.
void decode_mp4(const void *map, const int length);

// decode_box decodes the ISO/IEC 14496-12:2005(E) base Box class headers which
// determine the type and size of the box. Size calculation can vary.
// It returns the number of bytes that have been parsed in the process, in order
// to progress the mmap pointer.
int decode_box(void *map);

// it turns out that boxes that are just containers for other boxes all look the
// same, so we can just use the same function for them
void decode_nested_box(void *map, const struct BaseBox box);

// Following functions handle the different kind of mp4 boxes that might be found.
// These are "leaf" boxes without further nesting.
void decode_ftyp(void *map, const struct BaseBox box);

void decode_mdat(void *map, const struct BaseBox box);

void decode_mvhd(void *map, const struct BaseBox box);

void decode_tkhd(void *map, const struct BaseBox box);

void decode_mdhd(void *map, const struct BaseBox box);

void decode_hdlr(void *map, const struct BaseBox box);

// decode a timestamp - these are in seconds since midnight, Jan 1, 1904
// so convenient!
char *translate_timestamp(const u_int32_t timestamp);

void unknown_box_type(void *map, const struct BaseBox box);