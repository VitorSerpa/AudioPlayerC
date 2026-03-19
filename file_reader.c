#include "file_reader.h"

int file_reader_open(FileReader *file, const char *file_path){
    printf("%s", file_path);
    file->file_pointer = fopen(file_path, "rb");
    if(file->file_pointer == NULL) return -1;

    printf("passou");

    fseek(file->file_pointer, 0, SEEK_END);
    file->file_size = ftell(file->file_pointer);
    fseek(file->file_pointer, 0, SEEK_SET);
    return 0;
}

int file_reader_read_chunk(FileReader *file, uint8_t *buffer, size_t chunk_lenght){
    if(file->file_pointer == NULL) return -1;
    return fread(buffer, 1, chunk_lenght, file->file_pointer);
}

int file_reader_metadata(FileReader *file){
    fseek(file->file_pointer, 0, SEEK_SET);
    uint8_t buffer[200];
    fread(buffer, 1, 200, file->file_pointer);

    for(int i = 0; i < 200; i++){
        printf("%c", buffer[i]);
        if(i % 40 == 0) printf("\n");
    }
    printf("\n");
    
    return 0;
}

void file_reader_close(FileReader *file){
    if(file->file_pointer) fclose(file->file_pointer);
}