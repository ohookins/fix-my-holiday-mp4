#include <stdio.h>
#include <string.h>
#include "fix.h"
#include "compare.h"
#include "mp4.h"
#include "file.h"

void fix_broken_mp4(const char *broken_filename, const char *reference_filename, const char *output_filename) {
    int ref_size, ref_fd;
    int broken_size, broken_fd;
    void *ref_map = map_file(reference_filename, &ref_size, &ref_fd);
    void *broken_map = map_file(broken_filename, &broken_size, &broken_fd);

    BoxPath ref_boxes[MAX_BOXES], broken_boxes[MAX_BOXES];
    int ref_count = collect_boxes(ref_map, ref_size, ref_boxes, MAX_BOXES, "");
    int broken_count = collect_boxes(broken_map, broken_size, broken_boxes, MAX_BOXES, "");

    // List of box types that are static (can be copied directly)
    const char *static_boxes[] = {"ftyp", "mvhd", "tkhd", "mdhd", "hdlr", "minf", "vmhd", "dinf", "smhd", "stsd", "stsc", "stsz", "stco", "stbl", "udta", "SDLN", "smrd", "smta"};
    int num_static = sizeof(static_boxes) / sizeof(static_boxes[0]);

    // Open output file for writing
    FILE *out = fopen(output_filename, "wb");
    if (!out) {
        fprintf(stderr, "Error: Could not open output file %s for writing.\n", output_filename);
        cleanup(ref_map, ref_size, ref_fd);
        cleanup(broken_map, broken_size, broken_fd);
        return;
    }

    // 1. Write all boxes from the broken file (in order)
    for (int i = 0; i < broken_count; ++i) {
        const void *src_ptr = (char *)broken_map + broken_boxes[i].offset;
        int src_size = broken_boxes[i].size;
        size_t written = fwrite(src_ptr, 1, src_size, out);
        if (written != (size_t)src_size) {
            fprintf(stderr, "Error: Failed to write box %s from broken file (wanted %d, wrote %zu)\n", broken_boxes[i].path, src_size, written);
            fclose(out);
            cleanup(ref_map, ref_size, ref_fd);
            cleanup(broken_map, broken_size, broken_fd);
            return;
        }
        printf("Wrote box %s (broken, offset %d, size %d)\n", broken_boxes[i].path, broken_boxes[i].offset, src_size);
    }

    // 2. For each static box in the reference file that's missing from the broken file, append it (but never append mdat)
    int moov_found = 0;
    for (int i = 0; i < ref_count; ++i) {
        // Is this box missing from the broken file?
        int found = 0;
        for (int j = 0; j < broken_count; ++j) {
            if (strcmp(ref_boxes[i].path, broken_boxes[j].path) == 0) {
                found = 1;
                break;
            }
        }
        // Special handling for moov atom
        if (strncmp(ref_boxes[i].type, "moov", 4) == 0) {
            if (!found) {
                // Append the entire moov atom from the reference file
                const void *src_ptr = (char *)ref_map + ref_boxes[i].offset;
                int src_size = ref_boxes[i].size;
                size_t written = fwrite(src_ptr, 1, src_size, out);
                if (written != (size_t)src_size) {
                    fprintf(stderr, "Error: Failed to append missing moov atom from reference (wanted %d, wrote %zu)\n", src_size, written);
                    fclose(out);
                    cleanup(ref_map, ref_size, ref_fd);
                    cleanup(broken_map, broken_size, broken_fd);
                    return;
                }
                printf("Appended missing moov atom (reference, offset %d, size %d)\n", ref_boxes[i].offset, src_size);
            }
            moov_found = 1;
            continue;
        }
        // Never append mdat from the reference file
        if (!found && strncmp(ref_boxes[i].type, "mdat", 4) != 0) {
            // Only copy static boxes
            int is_static = 0;
            for (int k = 0; k < num_static; ++k) {
                if (strncmp(ref_boxes[i].type, static_boxes[k], 4) == 0) {
                    is_static = 1;
                    break;
                }
            }
            if (is_static) {
                const void *src_ptr = (char *)ref_map + ref_boxes[i].offset;
                int src_size = ref_boxes[i].size;
                size_t written = fwrite(src_ptr, 1, src_size, out);
                if (written != (size_t)src_size) {
                    fprintf(stderr, "Error: Failed to append missing static box %s from reference (wanted %d, wrote %zu)\n", ref_boxes[i].path, src_size, written);
                    fclose(out);
                    cleanup(ref_map, ref_size, ref_fd);
                    cleanup(broken_map, broken_size, broken_fd);
                    return;
                }
                printf("Appended missing static box %s (reference, offset %d, size %d)\n", ref_boxes[i].path, ref_boxes[i].offset, src_size);
            }
        }
    }

    fclose(out);
    cleanup(ref_map, ref_size, ref_fd);
    cleanup(broken_map, broken_size, broken_fd);
}
