void *map_file(const char *filename, int *outLength);

int openfile(const char *filename);

void *mmapfile(const int fd);

int getfilesize(const int fd);