#include "app.h"

int main(int argc, char **argv) { 
    if (argc <= 1) { 
        INFO("usage: wcl [FILE]");
        return -1; 
    }

    for (int i = 0; i < argc - 1; ++i) { 
        int file_ino = dir_lookup(workdir_ino, argv[i + 1]);
        if (file_ino < 0) { 
            INFO("wcl: file %s not found", argv[i + 1]);
            return -1; 
        }

        char buf[BLOCK_SIZE]; 
        char line[1024]; 
        int line_length = 0; 
        int line_count = 0; 
        int offset = 0;
        int keep_reading = 1;

        while (keep_reading) {
            file_read(file_ino, offset, buf); 
            offset++;

            for (int j = 0; j < BLOCK_SIZE; ++j) { 
                char current_character = buf[j]; 
                if (current_character == '\0') { 
                    keep_reading = 0; 
                    break;
                }

                char next_character = (j + 1 < BLOCK_SIZE) ? buf[j + 1] : '\0';

                if (current_character == '\n' ||
                   (current_character == '.' && (next_character == '\0' || next_character == ' '))) { 
                    line_count++; 
                    line_length = 0; 
                } else { 
                    if (line_length < 1023) { 
                        line[line_length++] = current_character;
                    }
                }
            }
        }

        if (line_length > 0) { 
            line_count++; 
        }

        printf("%d %s\n", line_count, argv[i + 1]); 
    }

    return 0;
}
