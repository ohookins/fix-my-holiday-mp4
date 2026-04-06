#include <stdio.h>
#include <getopt.h>

#include "file.h"
#include "mp4.h"
#include "compare.h"
#include <string.h>

void usage()
{
    printf("Usage: fix-my-holiday-mp4 -f FILENAME -m MODE [options]\n");
    printf("  -m print                Print MP4 metadata (default)\n");
    printf("  -m compare -c FILE      Compare with another MP4 file\n");
    printf("  -m fix -c FILE -o OUT   Fix using another MP4 file, write to OUT\n");
    printf("\n");
}

int main(const int argc, char **argv)
{
    // Parse command-line arguments
    char *filename = NULL;
    char *mode = "print";
    char *compare_file = NULL;
    char *output_file = NULL;
    int ch;

    // getopt string: f: for filename, m: for mode, c: for compare file, o: for output file
    while ((ch = getopt(argc, argv, "f:m:c:o:")) != -1)
    {
        switch (ch)
        {
        case 'f':
            filename = optarg;
            break;
        case 'm':
            mode = optarg;
            break;
        case 'c':
            compare_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        }
    }

    if (!filename)
    {
        usage();
        return 1;
    }

    if (strcmp(mode, "print") == 0)
    {
        // mmap the file for ease of access
        int filesize;
        int fd;
        void *map = map_file(filename, &filesize, &fd);

        // start parsing the memory range for mp4 boxes
        decode_mp4(map, filesize);

        // cleanup, assuming we didn't exit unexpectedly
        cleanup(map, filesize, fd);
    }
    else if (strcmp(mode, "compare") == 0)
    {
        if (!compare_file)
        {
            printf("Error: -m compare requires -c FILE parameter.\n");
            usage();
            return 1;
        }
        analyze_missing_boxes(filename, compare_file);
    }
    else if (strcmp(mode, "fix") == 0)
    {
        if (!compare_file || !output_file)
        {
            printf("Error: -mode fix requires -c FILE and -o OUT parameters.\n");
            usage();
            return 1;
        }
        printf("[STUB] Fix mode: would fix %s using %s and write to %s\n", filename, compare_file, output_file);
        // TODO: Implement fix logic
    }
    else
    {
        printf("Unknown mode: %s\n", mode);
        usage();
        return 1;
    }

    return 0;
}
