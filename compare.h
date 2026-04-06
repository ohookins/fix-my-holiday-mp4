#ifndef COMPARE_H
#define COMPARE_H

static void print_box_metadata(const char *type, const void *data, int size);

#define MAX_BOXES 256
typedef struct { char path[128]; char type[4]; int offset; int size; } BoxPath;

// Analyze missing boxes between two MP4 files
void analyze_missing_boxes(const char *target_filename, const char *reference_filename);

#endif // COMPARE_H
