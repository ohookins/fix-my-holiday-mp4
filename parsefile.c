#include <stdio.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>

#include "parsefile.h"

void parsefile(const char *filename)
{
    // open(2) the file
    int fd = openfile(filename);

    // mmap the file descriptor
    void *map = mmapfile(fd);

    // start parsing the memory range for mp4 boxes
}

int openfile(const char *filename)
{
    printf("Attempting to open \"%s\" for reading...\n", filename);

    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        printf("Error %d opening file: %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fd;
}

void *mmapfile(const int fd)
{
    // We need to know the file size in order to figure out how much memory to
    // map, otherwise we'll get EINVAL.
    int filesize = getfilesize(fd);

    // Probably won't be daring enough to make changes in place,
    // so this should stay PROT_READ for now. Actually would be a better
    // idea to write out a duplicate file with correcions rather than
    // edit in place anyway...
    void *map = mmap(NULL, (size_t)filesize, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);

    if (map == MAP_FAILED)
    {
        printf("Error %d mmaping file: %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return map;
}

int getfilesize(const int fd)
{
    struct stat st;

    int err = fstat(fd, &st);
    if (err < 0)
    {
        printf("Error %d getting file size: %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    int filesize = (int)st.st_size;

    printf("File size: %d\n", filesize);
    return filesize;
}