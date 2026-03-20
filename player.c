#include <stdlib.h>
#include <stdint.h>

#include "file_reader.h"
#define AUDIO_CHUNK_SIZE 1024

const char *directory_path = "D:\\GitHub\\AudioPlayerC\\audios\\music_list.txt";
char buffer[50];

// int listMusics(const char *directorypath){
//     FILE* music_list_file;
//     music_list_file = fopen(directory_path, "r");
//     while(fgets(buffer, 100, music_list_file)) {
//         printf("WHILE");
//         printf("%s", buffer);
//     }
//     return 0;
// }

int main(int argc, char *argv[]){
    FileReader* file = malloc(sizeof(FileReader));  
    
    if(file_reader_open(file, "D:\\GitHub\\AudioPlayerC\\audios\\Radiohead - Jigsaw Falling Into Place.mp3") != 0) return -1;
    
    //file_reader_metadata(file);

    return 0;
}
