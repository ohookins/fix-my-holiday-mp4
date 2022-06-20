#include <stdbool.h>

enum mp4_size_flag
{
    COMPACT,
    EXTENDED,
};

struct Box
{
    char type[4];
    enum mp4_size_flag size_flag;
    union
    {
        u_int32_t compact;
        u_int64_t extended;
    } size;
    u_int8_t extended_type[16];
};

// decode_mp4 starts the overall process of decoding the MP4 file.
void decode_mp4(const void *map, const int length);

// decode_box decodes the ISO/IEC 14496-12:2005(E) base Box class headers which
// determine the type and size of the box. Size calculation can vary.
// It returns the number of bytes that have been parsed in the process, in order
// to progress the mmap pointer.
int decode_box(void *map);

// Following functions handle the different kind of mp4 boxes that might be found.
void decode_ftyp(void *map, const struct Box box);

void decode_mdat(void *map, const struct Box box);
