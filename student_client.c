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


int EnrollCourse(int client_sock) {
    char username[MAX_USERNAME_LENGTH], course_id[MAX_COURSE_ID_LENGTH];
    char response[128];
    int bytes_read;

    printf("\nEnter your username: ");
    scanf("%24s", username);
    clear_input_buffer();
    write(client_sock, username, strlen(username));

    printf("Enter course ID: ");
    scanf("%9s", course_id);
    clear_input_buffer();
    write(client_sock, course_id, strlen(course_id));

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

int UnenrollCourse(int client_sock) {
    char username[MAX_USERNAME_LENGTH], course_id[MAX_COURSE_ID_LENGTH];
    char response[128];
    int bytes_read;

    printf("\nEnter your username: ");
    scanf("%24s", username);
    clear_input_buffer();
    write(client_sock, username, strlen(username));

    printf("Enter course ID: ");
    scanf("%9s", course_id);
    clear_input_buffer();
    write(client_sock, course_id, strlen(course_id));

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

int ViewEnrolledCourses(int client_sock) {
    char username[MAX_USERNAME_LENGTH];
    char response[128];
    int bytes_read;

    printf("\nEnter your username: ");
    scanf("%24s", username);
    clear_input_buffer();
    write(client_sock, username, strlen(username));

    printf("\n--- Enrolled Courses ---\n");

    struct pollfd pfd;
    pfd.fd = client_sock;
    pfd.events = POLLIN;
    while (1) {
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

        if (strcmp(response, "End of list\n") == 0) {
            printf("\n-----------------------\n\n");
            break;
        }

        // Extract course ID from "Course ID: CS101\n"
        char course_id[MAX_COURSE_ID_LENGTH];
        if (sscanf(response, "Course ID: %9s", course_id) == 1) {
            printf("| %-15s |\n", course_id);
        } else {
            printf("\nServer Response: %s\n", response);
        }
    }
    return 1;
}
