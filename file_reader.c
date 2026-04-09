#include "file_reader.h"
#include "minimp3.h"
int file_reader_open(FileReader *file, const char *file_path){
    file->file_pointer = fopen(file_path, "rb");

    if(file->file_pointer == NULL) return -1;

    fseek(file->file_pointer, 0, SEEK_END);
    file->file_size = ftell(file->file_pointer);

    fseek(file->file_pointer, 0, SEEK_SET);
    return 0;
}

int file_reader_read_chunk(FileReader *file, uint16_t *buffer, size_t chunk_lenght){
    if(file->file_pointer == NULL) return -1;
    return fread(buffer, 1, chunk_lenght, file->file_pointer);
}

void file_reader_metadata(FileReader *file){
    fseek(file->file_pointer, 0, SEEK_SET);
    uint8_t buffer[500];
    uint8_t header[10];

    if(memcmp(header, "ID3", 3) != 0) return -1;

    fread(buffer, 1, 500, file->file_pointer);

    for(int i = 0; i < 280; i++){
        printf("%c", buffer[i]);
        if(i % 40 == 0);
    }
    printf("\n");
}

void file_reader_close(FileReader *file){
    if(file->file_pointer) fclose(file->file_pointer);
}