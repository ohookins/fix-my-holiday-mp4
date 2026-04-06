#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "compare.h"
#include "file.h"
#include "mp4.h"


#define MAX_BOXES 32
typedef struct { char type[4]; } BoxType;

// Helper: enumerate top-level boxes and store their types
static int collect_box_types(const void *map, int filesize, BoxType *out_types, int max_types) {
    int count = 0;
    void *current = (void *)map;
    void *end = (void *)(map + filesize);
    while (current < end && count < max_types) {
        struct BaseBox box;
        memcpy(&box, current, sizeof(box));
        uint32_t size = ntohl(box.size);
        memcpy(out_types[count].type, box.type, 4);
        count++;
        if (size == 0 || size > (end - current)) break;
        current += size;
    }
    return count;
}

void analyze_missing_boxes(const char *target_filename, const char *reference_filename) {
    int target_size, target_fd;
    int ref_size, ref_fd;
    void *target_map = map_file(target_filename, &target_size, &target_fd);
    void *ref_map = map_file(reference_filename, &ref_size, &ref_fd);

    BoxType target_boxes[MAX_BOXES], ref_boxes[MAX_BOXES];
    int target_count = collect_box_types(target_map, target_size, target_boxes, MAX_BOXES);
    int ref_count = collect_box_types(ref_map, ref_size, ref_boxes, MAX_BOXES);

    printf("Top-level boxes in %s:\n", target_filename);
    for (int i = 0; i < target_count; ++i) {
        printf("  %.4s\n", target_boxes[i].type);
    }
    printf("Top-level boxes in %s:\n", reference_filename);
    for (int i = 0; i < ref_count; ++i) {
        printf("  %.4s\n", ref_boxes[i].type);
    }

    // Compare and print missing boxes
    printf("\nBoxes present in %s but missing from %s:\n", target_filename, reference_filename);
    for (int i = 0; i < target_count; ++i) {
        int found = 0;
        for (int j = 0; j < ref_count; ++j) {
            if (memcmp(target_boxes[i].type, ref_boxes[j].type, 4) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("  %.4s\n", target_boxes[i].type);
        }
    }

    cleanup(target_map, target_size, target_fd);
    cleanup(ref_map, ref_size, ref_fd);
}
