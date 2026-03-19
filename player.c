#include <stdlib.h>
#include <stdint.h>

#include "file_reader.h"

#define AUDIO_CHUNK_SIZE 1024

int listMusics(const char *directorypath){
    
}

int main(int argc, char *argv[]){
    FileReader* file = malloc(sizeof(FileReader));



    if(file_reader_open(file, "D:\\GitHub\\AudioPlayerC\\audios\\Radiohead - Jigsaw Falling Into Place.mp3") != 0) return -1;
    
    file_reader_metadata(file);

    return 0;
}
