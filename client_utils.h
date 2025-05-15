#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

void clear_input_buffer(void);
void print_response(const char *response);
int is_valid_role(const char *role);
int is_valid_choice(const char *choice, const char *user_type);

#endif