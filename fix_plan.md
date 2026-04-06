# Implementation Plan for fix_broken_mp4

## Goal
- Actually write a fixed MP4 file by copying missing static boxes from the reference file to the broken file, in the correct order and hierarchy.

## Steps
1. Open the output file for writing (truncate or create).
2. For each box in the reference file:
    a. If the box exists in the broken file, copy it from the broken file.
    b. If the box is missing but is a static box, copy it from the reference file.
    c. If the box is missing and not static, skip for now (future: estimate or reconstruct).
3. Write each box in order to the output file, preserving hierarchy.
4. Close all files and clean up.

## Implementation Details
- Use mmap for input files, but use write() for output file.
- For each box, use the offset/size from the appropriate file's mmap.
- For now, only handle top-level boxes and static boxes (no estimation).
- Print what is being written for debugging.
- Error handling: if write fails, print error and abort.

## Next Steps
- Implement step 1: open output file for writing.
- Implement step 2: iterate boxes, decide source, and write.
- Implement step 3: close and cleanup.
- Test with real files.
