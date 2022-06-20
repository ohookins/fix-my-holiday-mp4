#include <stdio.h>
#include <getopt.h>

#include "file.h"
#include "mp4.h"

void usage()
{
    printf("Usage: fix-my-holiday-mp4 -f FILENAME\n\n");
}

int main(const int argc, const char **argv)
{
    // Parse command-line arguments
    char *filename;
    int ch;

    while ((ch = getopt(argc, argv, "f:")) != -1)
    {
        switch (ch)
        {
        case 'f':
            filename = optarg;
            break;
        }
    }

    if (!filename)
    {
        usage();
        return 1;
    }

    // mmap the file for ease of access
    int filesize;
    void *map = map_file(filename, &filesize);

    // start parsing the memory range for mp4 boxes
    decode_mp4(map, filesize);
}
