#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "compare.h"
#include "file.h"
#include "mp4.h"



#define MAX_BOXES 256
typedef struct { char path[128]; char type[4]; } BoxPath;

// Recursively collect all boxes (with hierarchy path)
static int collect_boxes(const void *map, int filesize, BoxPath *out_boxes, int max_boxes, const char *parent_path) {
    int count = 0;
    void *current = (void *)map;
    void *end = (void *)(map + filesize);
    while (current < end && count < max_boxes) {
        struct BaseBox box;
        memcpy(&box, current, sizeof(box));
        uint32_t size = ntohl(box.size);
        char path[128];
        if (parent_path && parent_path[0])
            snprintf(path, sizeof(path), "%s/%.4s", parent_path, box.type);
        else
            snprintf(path, sizeof(path), "%.4s", box.type);
        memcpy(out_boxes[count].path, path, sizeof(out_boxes[count].path));
        memcpy(out_boxes[count].type, box.type, 4);
        count++;
        // If this is a known container box, recurse
        if (strncmp(box.type, "moov", 4) == 0 || strncmp(box.type, "trak", 4) == 0 ||
            strncmp(box.type, "mdia", 4) == 0 || strncmp(box.type, "minf", 4) == 0 ||
            strncmp(box.type, "stbl", 4) == 0 || strncmp(box.type, "udta", 4) == 0) {
            int inner = collect_boxes(current + 8, size - 8, out_boxes + count, max_boxes - count, path);
            count += inner;
        }
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


    BoxPath target_boxes[MAX_BOXES], ref_boxes[MAX_BOXES];
    int target_count = collect_boxes(target_map, target_size, target_boxes, MAX_BOXES, "");
    int ref_count = collect_boxes(ref_map, ref_size, ref_boxes, MAX_BOXES, "");

    printf("All boxes in %s:\n", target_filename);
    for (int i = 0; i < target_count; ++i) {
        printf("  %s\n", target_boxes[i].path);
    }
    printf("All boxes in %s:\n", reference_filename);
    for (int i = 0; i < ref_count; ++i) {
        printf("  %s\n", ref_boxes[i].path);
    }


    // List of box types that are static (can be copied directly)
    const char *static_boxes[] = {"ftyp", "mvhd", "tkhd", "mdhd", "hdlr", "minf", "vmhd", "dinf", "smhd", "stsd", "stsc", "stsz", "stco", "stbl", "udta", "SDLN", "smrd", "smta"};
    int num_static = sizeof(static_boxes) / sizeof(static_boxes[0]);

    // List of box types that are dynamic (need estimation)
    const char *dynamic_boxes[] = {"stts", "stss", "mdat"};
    int num_dynamic = sizeof(dynamic_boxes) / sizeof(dynamic_boxes[0]);

    printf("\nBoxes present in %s but missing from %s:\n", target_filename, reference_filename);
    for (int i = 0; i < target_count; ++i) {
        int found = 0;
        for (int j = 0; j < ref_count; ++j) {
            if (strcmp(target_boxes[i].path, ref_boxes[j].path) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            // Determine classification
            const char *type = target_boxes[i].type;
            const char *class = "unknown";
            for (int k = 0; k < num_static; ++k) {
                if (strncmp(type, static_boxes[k], 4) == 0) {
                    class = "copy";
                    break;
                }
            }
            for (int k = 0; k < num_dynamic; ++k) {
                if (strncmp(type, dynamic_boxes[k], 4) == 0) {
                    class = "estimate";
                    break;
                }
            }
            printf("  %s [%s]\n", target_boxes[i].path, class);
        }
    }

    cleanup(target_map, target_size, target_fd);
    cleanup(ref_map, ref_size, ref_fd);
}
