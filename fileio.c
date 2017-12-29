#include <stdio.h>
#include <stdlib.h>
#include "fileio.h"

int read_file(char *filename, uint8_t **buf, size_t *size) {
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to load file (\"%s\").\n", filename);
        return 1;
    }

    /* Get the file size */
    fseek(fp, 0L, SEEK_END);
    size_t allocsize = ftell(fp) * sizeof(uint8_t);
    rewind(fp);

    *buf = (uint8_t *)malloc(allocsize);
    if (*buf == NULL) {
        fprintf(stderr,
                "Error allocating mem for file (file=%s, size=%zu byte).",
                filename, allocsize);
        fclose(fp);
        return 1;
    }
    *size = fread(*buf, sizeof(uint8_t), allocsize, fp);
    fclose(fp);
    return 0;
}

int save_file(char *filename, uint8_t *buf, size_t size) {
    FILE *fp;

    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file (\"%s\").\n", filename);
        return 1;
    }

    fwrite(buf, sizeof(uint8_t), size, fp);
    fclose(fp);
    return 0;
}

