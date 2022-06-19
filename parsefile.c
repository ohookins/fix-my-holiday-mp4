#include <stdio.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <string.h>

#include "parsefile.h"

void parsefile(const char *filename)
{
    // open(2) the file
    int fd = openfile(filename);

    // mmap the file descriptor

    // start parsing the memory range for mp4 boxes
}

int openfile(const char *filename)
{
    printf("Attempting to open \"%s\" for reading...\n", filename);

    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        printf("Error %d opening file: %s\n", errno, strerror(errno));
        exit(1);
    }

    return fd;
}