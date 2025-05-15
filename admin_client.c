#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <poll.h>
#include "client_utils.h"

#define MAX_USERNAME_LENGTH 25
#define MAX_PASSWORD_LENGTH 25
#define MAX_NAME_LENGTH 50
#define MAX_COURSE_ID_LENGTH 10
#define MAX_COURSE_NAME_LENGTH 50
#define MAX_ROLE_LENGTH 10
#define MAX_ACTION_LENGTH 20
#define MAX_CHOICE_LENGTH 5

int AddStudent(int client_sock) {
    char username[MAX_USERNAME_LENGTH], name[MAX_NAME_LENGTH], password[MAX_PASSWORD_LENGTH];
    char response[128];
    int bytes_read;

    printf("\nEnter student username: ");
    scanf("%24s", username);
    clear_input_buffer();
    write(client_sock, username, strlen(username));

    printf("Enter student name: ");
    fgets(name, MAX_NAME_LENGTH, stdin);
    name[strcspn(name, "\n")] = 0;
    write(client_sock, name, strlen(name));

    printf("Enter student password: ");
    scanf("%24s", password);
    clear_input_buffer();
    write(client_sock, password, strlen(password));

    struct pollfd pfd;
    pfd.fd = client_sock;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, 5000);
    if (ret <= 0) {
        printf("\nServer Response: Timeout or error waiting for server response\n\n");
        return 1;
    }

    bytes_read = read(client_sock, response, sizeof(response) - 1);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read server response");
        else printf("\nServer Response: Server disconnected\n\n");
        return 1;
    }
    response[bytes_read] = '\0';
    print_response(response);
    return 1;
}

int AddFaculty(int client_sock) {
    char username[MAX_USERNAME_LENGTH], name[MAX_NAME_LENGTH], password[MAX_PASSWORD_LENGTH];
    char response[128];
    int bytes_read;

    printf("\nEnter faculty username: ");
    scanf("%24s", username);
    clear_input_buffer();
    write(client_sock, username, strlen(username));

    printf("Enter faculty name: ");
    fgets(name, MAX_NAME_LENGTH, stdin);
    name[strcspn(name, "\n")] = 0;
    write(client_sock, name, strlen(name));

    printf("Enter faculty password: ");
    scanf("%24s", password);
    clear_input_buffer();
    write(client_sock, password, strlen(password));

    struct pollfd pfd;
    pfd.fd = client_sock;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, 5000);
    if (ret <= 0) {
        printf("\nServer Response: Timeout or error waiting for server response\n\n");
        return 1;
    }

    bytes_read = read(client_sock, response, sizeof(response) - 1);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read server response");
        else printf("\nServer Response: Server disconnected\n\n");
        return 1;
    }
    response[bytes_read] = '\0';
    print_response(response);
    return 1;
}

int ToggleStudentStatus(int client_sock) {
    char username[MAX_USERNAME_LENGTH];
    char response[128];
    int bytes_read;

    printf("\nEnter student username: ");
    scanf("%24s", username);
    clear_input_buffer();
    write(client_sock, username, strlen(username));

    struct pollfd pfd;
    pfd.fd = client_sock;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, 5000);
    if (ret <= 0) {
        printf("\nServer Response: Timeout or error waiting for server response\n\n");
        return 1;
    }

    bytes_read = read(client_sock, response, sizeof(response) - 1);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read server response");
        else printf("\nServer Response: Server disconnected\n\n");
        return 1;
    }
    response[bytes_read] = '\0';
    print_response(response);
    return 1;
}

int UpdateDetails(int client_sock) {
    char role[MAX_ROLE_LENGTH], username[MAX_USERNAME_LENGTH], new_name[MAX_NAME_LENGTH];
    char response[128];
    int bytes_read;

    printf("\n");
    while (1) {
        printf("Enter role (admin/student/faculty): ");
        scanf("%9s", role);
        clear_input_buffer();
        if (is_valid_role(role)) {
            break;
        }
        printf("Invalid role. Choose 'admin', 'student', or 'faculty'\n");
    }
    write(client_sock, role, strlen(role));

    printf("Enter username: ");
    scanf("%24s", username);
    clear_input_buffer();
    write(client_sock, username, strlen(username));

    printf("Enter new name: ");
    fgets(new_name, MAX_NAME_LENGTH, stdin);
    new_name[strcspn(new_name, "\n")] = 0;
    write(client_sock, new_name, strlen(new_name));

    struct pollfd pfd;
    pfd.fd = client_sock;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, 5000);
    if (ret <= 0) {
        printf("\nServer Response: Timeout or error waiting for server response\n\n");
        return 1;
    }

    bytes_read = read(client_sock, response, sizeof(response) - 1);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read server response");
        else printf("\nServer Response: Server disconnected\n\n");
        return 1;
    }
    response[bytes_read] = '\0';
    print_response(response);
    return 1;
}