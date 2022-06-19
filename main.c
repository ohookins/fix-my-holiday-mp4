#include <stdio.h>
#include <getopt.h>

void usage()
{
    printf("Usage: fix-my-holiday-mp4 -f FILENAME\n\n");
}

int main(int argc, char **argv)
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

    printf("Filename was %s\n", filename);
}
