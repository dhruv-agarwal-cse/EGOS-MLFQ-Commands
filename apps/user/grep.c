#include "app.h"

int main(int argc, char** argv){ 
    if(argc != 3){ 
        printf("%s\n", "argc");
        INFO("usage: grep [PATTERN] [FILE]"); 
        return -1; 
    }

    int file_ino = dir_lookup(workdir_ino, argv[2]); 
    if(file_ino < 0){ 
        printf("%s\n","fileino");
        INFO("grep: file %s not found", argv[2]); 
        return -1;
    }

    char buf[BLOCK_SIZE]; 
    char line[BLOCK_SIZE];
    int  line_length = 0;

    int keep_reading = 1;
    int block = 0;

    while (keep_reading) {
        file_read(file_ino, block, buf);
        block++;

        for (int i = 0; i < BLOCK_SIZE && keep_reading; ++i) {
            char current_character = buf[i];
            char next_character    = (i + 1 < BLOCK_SIZE) ? buf[i + 1] : '\0';

            if (current_character == '\0') {
                if (line_length > 0) {
                    line[line_length] = '\0';
                    if (strstr(line, argv[1]) != NULL) {
                        printf("%s\n", line);
                    }
                }
                keep_reading = 0;
                break;
            }

            if (current_character == '\n') {
                line[line_length] = '\0';
                if (strstr(line, argv[1]) != NULL) {
                    printf("%s\n", line);
                }
                line_length = 0;
            }

            else if (current_character == '.') {
                int only_spaces_or_null = 1;
                for (int j = i + 1; j < BLOCK_SIZE; ++j) {
                    if (buf[j] == '\0') {
                        break;
                    }
                    if (buf[j] != ' ') {
                        only_spaces_or_null = 0;
                        break;
                    }
                }

                line[line_length] = '\0';
                if (strstr(line, argv[1]) != NULL) {
                    printf("%s\n", line);
                }
                line_length = 0;

                if (only_spaces_or_null) {
                    keep_reading = 0;
                    break;
                }
            }

            else {
                if (line_length < BLOCK_SIZE - 1) {
                    line[line_length++] = current_character;
                }
            }
        }
    }

    if (line_length > 0) {
        line[line_length] = '\0';
        if (strstr(line, argv[1]) != NULL) {
            printf("%s\n", line);
        }
    }

    return 0;
}
