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
        int offset = 0;
        int line_count = 0;
        int keep_reading = 1;

        while (keep_reading) {
            file_read(file_ino, offset, buf);
            offset++;

            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (buf[j] == '\0') {  
                    keep_reading = 0;
                    break;
                }
                if (buf[j] == '\n') {      
                    line_count++;
                }
            }
        }

        printf("%d %s\n", line_count, argv[i + 1]);
    }

    return 0;
}
