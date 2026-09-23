#include "libsysy.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_BUFFER_LEN 512

char get_char() {
    int value = getchar();

    assert(value >= 0);

    return (char)value;
}

int get_int(void) {
    char input_buffer[INPUT_BUFFER_LEN];

    if (fgets(input_buffer, INPUT_BUFFER_LEN, stdin) == NULL) {
        fprintf(stderr, "libsys: get_int: EOF reached\n");
        abort();
    }

    char *p = input_buffer;

    while (isspace(*p)) {
        p++;
    }

    if (*p == '\0') {
        fprintf(stderr, "libsys: get_int: empty line\n");
        abort();
    }

    errno = 0;
    char *end_ptr = NULL;
    long value = strtol(p, &end_ptr, 10);

    if (p == end_ptr) {
        fprintf(stderr, "libsys: get_int: invalid digits\n");
        abort();
    }

    while (isspace(*end_ptr)) {
        end_ptr++;
    }

    if (*end_ptr != '\0') {
        fprintf(stderr, "libsys: get_int: invalid trailing characters\n");
        abort();
    }

    if (errno == ERANGE || value < INT_MIN || value > INT_MAX) {
        fprintf(stderr, "libsys: get_int: integer out of range\n");
        abort();
    }

    return (int)value;
}

void get_string(char *buffer, int max_len) {
    int stored_length = 0;
    int max_stored_length = max_len > 0 ? max_len - 1 : 0;
    int read_anything = 0;
    int value;

    while ((value = fgetc(stdin)) != EOF) {
        read_anything = 1;

        if (value == '\n') {
            break;
        }

        if (value == '\r') {
            int next_value = fgetc(stdin);

            if (next_value == EOF) {
                if (ferror(stdin)) {
                    fprintf(stderr, "libsys: get_string: input error: %s\n", strerror(errno));
                    abort();
                }
            } else if (next_value != '\n' && ungetc(next_value, stdin) == EOF) {
                fprintf(stderr, "libsys: get_string: failed to restore input character\n");
                abort();
            }
            break;
        }

        if (stored_length < max_stored_length) {
            buffer[stored_length] = (char)value;
            stored_length++;
        }
    }

    if (value == EOF) {
        if (ferror(stdin)) {
            fprintf(stderr, "libsys: get_string: input error: %s\n", strerror(errno));
            abort();
        }
        if (!read_anything) {
            fprintf(stderr, "libsys: get_string: EOF reached\n");
            abort();
        }
    }

    if (max_len <= 0) {
        return;
    }

    if (stored_length < max_stored_length) {
        buffer[stored_length] = '\n';
        stored_length++;
    }

    buffer[stored_length] = '\0';
}

void put_int(int a) {
    printf("%d", a);
}

void put_char(char a) {
    printf("%c", a);
}

void put_string(char *str) {
    printf("%s", str);
}
