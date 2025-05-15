#include <stdio.h>
#include <string.h>
#include "client_utils.h"

void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void print_response(const char *response) {
    printf("\nServer Response: %s\n", response);
}

int is_valid_role(const char *role) {
    return strcmp(role, "admin") == 0 || strcmp(role, "student") == 0 || strcmp(role, "faculty") == 0;
}

int is_valid_choice(const char *choice, const char *user_type) {
    if (strcmp(user_type, "admin") == 0 || strcmp(user_type, "faculty") == 0 || strcmp(user_type, "student") == 0) {
        return strcmp(choice, "1") == 0 || strcmp(choice, "2") == 0 ||
               strcmp(choice, "3") == 0 || strcmp(choice, "4") == 0 ||
               strcmp(choice, "5") == 0;
    }
    return 0;
}