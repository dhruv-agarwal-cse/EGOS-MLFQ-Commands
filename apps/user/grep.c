#include "app.h"
#include <string.h>


static void get_and_print_line(int file_inode_number, int start_byte_position) {
    char read_buffer[BLOCK_SIZE];
    char print_buffer[BLOCK_SIZE];
    int print_buffer_length = 0;

    int current_block = start_byte_position / BLOCK_SIZE;
    int offset_in_block = start_byte_position % BLOCK_SIZE;

    while (1) {
        file_read(file_inode_number, current_block, read_buffer);
        current_block = current_block + 1;

        int i = offset_in_block;
        while (i < BLOCK_SIZE) {
            char current_char = read_buffer[i];

            if (current_char == '\0' || current_char == '\n') {
                if (print_buffer_length > 0) {
                    print_buffer[print_buffer_length] = '\0';
                    printf("%s", print_buffer);
                    print_buffer_length = 0;
                }
                printf("\n");
                return;
            }

            print_buffer[print_buffer_length] = current_char;
            print_buffer_length = print_buffer_length + 1;

            if (print_buffer_length == BLOCK_SIZE - 1) {
                print_buffer[print_buffer_length] = '\0';
                printf("%s", print_buffer);
                print_buffer_length = 0;
            }
            i = i + 1;
        }
        offset_in_block = 0;
    }
}


int main(int argc, char **argv) {
    if (argc != 3) {
        INFO("usage: grep [PATTERN] [FILE]");
        return -1;
    }

    const char *search_pattern = argv[1];
    int pattern_length = (int)strlen(search_pattern);

    int file_inode = dir_lookup(workdir_ino, argv[2]);
    if (file_inode < 0) {
        INFO("grep: file %s not found", argv[2]);
        return -1;
    }

    if (pattern_length == 0) {
        INFO("grep: empty pattern not supported");
        return -1;
    }

    if (pattern_length > BLOCK_SIZE) {
        INFO("grep: pattern too long (max %d)", BLOCK_SIZE);
        return -1;
    }

    char block_buffer[BLOCK_SIZE];
    char sliding_window[BLOCK_SIZE];
    int window_length = 0;

    int current_block_index = 0;
    int is_reading = 1;
    int current_line_has_match = 0;
    int line_start_position = 0;
    int global_file_position = 0;

    while (is_reading) {
        file_read(file_inode, current_block_index, block_buffer);
        current_block_index = current_block_index + 1;

        int block_offset = 0;
        while (block_offset < BLOCK_SIZE && is_reading) {
            char current_char = block_buffer[block_offset];

            if (current_char == '\0') {
                if (current_line_has_match) {
                    get_and_print_line(file_inode, line_start_position);
                }
                is_reading = 0;
                break;
            }

            if (current_char == '\n') {
                if (current_line_has_match) {
                    get_and_print_line(file_inode, line_start_position);
                }

                global_file_position = global_file_position + 1;
                line_start_position = global_file_position;
                current_line_has_match = 0;
                window_length = 0;
                block_offset = block_offset + 1;
                continue;
            }

            if (!current_line_has_match) {
                if (window_length < pattern_length) {
                    sliding_window[window_length] = current_char;
                    window_length = window_length + 1;
                } else {
                    int k = 1;
                    while (k < pattern_length) {
                        sliding_window[k - 1] = sliding_window[k];
                        k = k + 1;
                    }
                    sliding_window[pattern_length - 1] = current_char;
                }

                if (window_length == pattern_length) {
                    int is_fully_matched = 1;
                    int k = 0;
                    while (k < pattern_length) {
                        if (sliding_window[k] != search_pattern[k]) {
                            is_fully_matched = 0;
                            break;
                        }
                        k = k + 1;
                    }
                    if (is_fully_matched) {
                        current_line_has_match = 1;
                    }
                }
            }
            global_file_position = global_file_position + 1;
            block_offset = block_offset + 1;
        }
    }

    return 0;
}
