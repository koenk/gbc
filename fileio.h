#ifndef FILEIO_H
#define FILEIO_H

#include <stddef.h>
#include <stdint.h>

int read_file(char *filename, uint8_t **buf, size_t *size);
int save_file(char *filename, uint8_t *buf, size_t size);

#endif
