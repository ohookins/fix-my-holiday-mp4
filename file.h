// map_file was supposed to be a simple wrapper around opening a file, mmaping it
// etc but it turns out that for cleaning up and other operations we need things
// like the file length and file descriptor so we need to return these as out
// parameters as well.
void *map_file(const char *filename, int *outLength, int *outFd);

int openfile(const char *filename);

void *mmapfile(const int fd);

int getfilesize(const int fd);

// unmap the file and close the file descriptor to be nice
void cleanup(void *map, const int filesize, const int fd);