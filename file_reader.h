#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    FILE *file_pointer;
    size_t file_size;
    
} FileReader;

int file_reader_open(FileReader *file, const char *file_path);
int file_reader_read_chunk(FileReader *file, uint8_t *buffer, size_t chunk_lenght);
void file_reader_metadata(FileReader *file);
void read_frame_string(uint8_t *data, int size, char *out, int out_size);
void file_reader_close(FileReader *file);

