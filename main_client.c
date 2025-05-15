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

void clear_input_buffer(void);
void print_response(const char *response);
int is_valid_role(const char *role);
int is_valid_choice(const char *choice, const char *user_type);
void AddStudent(int client_sock);
void AddFaculty(int client_sock);
void ToggleStudentStatus(int client_sock);
void UpdateDetails(int client_sock);
void AddCourse(int client_sock);
void RemoveCourse(int client_sock);
void ViewCourseEnrollments(int client_sock);
void EnrollCourse(int client_sock);
void UnenrollCourse(int client_sock);
void ViewEnrolledCourses(int client_sock);

int ChangePassword(int client_sock) {
    char username[MAX_USERNAME_LENGTH], new_password[MAX_PASSWORD_LENGTH];
    char response[128];
    int bytes_read;

    printf("\nEnter your username: ");
    scanf("%24s", username);
    clear_input_buffer();
    write(client_sock, username, strlen(username));
    printf("Sent username: %s\n", username);

    printf("Enter new password: ");
    scanf("%24s", new_password);
    clear_input_buffer();
    write(client_sock, new_password, strlen(new_password));
    printf("Sent new password: %s\n", new_password);

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

int main() {
    struct sockaddr_in server_addr;
    int client_sock;
    char action[MAX_ACTION_LENGTH], role[MAX_ROLE_LENGTH], username[MAX_USERNAME_LENGTH], password[MAX_PASSWORD_LENGTH], name[MAX_NAME_LENGTH];
    char choice[MAX_CHOICE_LENGTH];
    char response[128];
    int bytes_read;
    int is_authenticated = 0;
    int is_admin = 0, is_faculty = 0;

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }
    printf("Client socket created\n\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(9050);

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to Course Management System\n\n");

    printf("\n====================================================\n");
    printf("      Welcome to Course Management : Academia      \n");
    printf("====================================================\n\n");

    while (!is_authenticated) {
        printf("----------------------------------------\n");
        printf("=== Course Management System ===\n");
        printf("----------------------------------------\n");
        printf("1) Login\n");
        printf("2) Register Admin\n");
        printf("3) Exit\n");
        printf("----------------------------------------\n");
        printf("Enter your choice (1/2/3): ");
        scanf("%4s", choice);
        clear_input_buffer();

        if (strcmp(choice, "3") == 0) {
            write(client_sock, "exit", strlen("exit"));
            close(client_sock);
            printf("\nExiting system. Goodbye!\n");
            exit(0);
        }

        if (strcmp(choice, "2") == 0) {
            write(client_sock, "register_admin", strlen("register_admin"));
            printf("\n---------- Register Admin ----------\n");
            printf("Enter username: ");
            scanf("%24s", username);
            clear_input_buffer();
            write(client_sock, username, strlen(username));

            printf("Enter password: ");
            scanf("%24s", password);
            clear_input_buffer();
            write(client_sock, password, strlen(password));

            printf("Enter name: ");
            fgets(name, MAX_NAME_LENGTH, stdin);
            name[strcspn(name, "\n")] = 0;
            write(client_sock, name, strlen(name));

            struct pollfd pfd;
            pfd.fd = client_sock;
            pfd.events = POLLIN;
            int ret = poll(&pfd, 1, 5000);
            if (ret <= 0) {
                printf("\nServer Response: Timeout or error waiting for server response\n\n");
                continue;
            }

            bytes_read = read(client_sock, response, sizeof(response) - 1);
            if (bytes_read <= 0) {
                if (bytes_read < 0) perror("Failed to read server response");
                else printf("\nServer Response: Server disconnected\n\n");
                continue;
            }
            response[bytes_read] = '\0';
            print_response(response);

            if (strcmp(response, "Registration successful\n") == 0) {
                is_authenticated = 1;
                is_admin = 1;
            }
        } else if (strcmp(choice, "1") == 0) {
            printf("\n------------- Login -------------\n");
            while (1) {
                printf("Enter role (admin/student/faculty): ");
                scanf("%9s", role);
                clear_input_buffer();
                if (is_valid_role(role)) break;
                printf("Invalid role. Choose 'admin', 'student', or 'faculty'\n");
            }
            write(client_sock, "login", strlen("login"));

            printf("Enter username: ");
            scanf("%24s", username);
            clear_input_buffer();
            write(client_sock, username, strlen(username));

            printf("Enter password: ");
            scanf("%24s", password);
            clear_input_buffer();
            write(client_sock, password, strlen(password));

            printf("Enter role (admin/student/faculty): ");
            scanf("%9s", role);
            clear_input_buffer();
            write(client_sock, role, strlen(role));

            struct pollfd pfd;
            pfd.fd = client_sock;
            pfd.events = POLLIN;
            int ret = poll(&pfd, 1, 5000);
            if (ret <= 0) {
                printf("\nServer Response: Timeout or error waiting for server response\n\n");
                continue;
            }

            bytes_read = read(client_sock, response, sizeof(response) - 1);
            if (bytes_read <= 0) {
                if (bytes_read < 0) perror("Failed to read server response");
                else printf("\nServer Response: Server disconnected\n\n");
                continue;
            }
            response[bytes_read] = '\0';
            print_response(response);

            if (strcmp(response, "admin login successful\n") == 0) {
                is_authenticated = 1;
                is_admin = 1;
            } else if (strcmp(response, "student login successful\n") == 0) {
                is_authenticated = 1;
            } else if (strcmp(response, "faculty login successful\n") == 0) {
                is_authenticated = 1;
                is_faculty = 1;
            }
        } else {
            printf("\nInvalid choice. Please enter 1, 2, or 3.\n\n");
        }
    }

    if (is_admin) {
        while (1) {
            printf("\n=== Admin Dashboard ===\n");
            printf("1) Add student\n");
            printf("2) Add faculty\n");
            printf("3) Activate/Deactivate student\n");
            printf("4) Update student/faculty details\n");
            printf("5) Exit\n");
            printf("Enter your choice: ");
            scanf("%4s", choice);
            clear_input_buffer();
            printf("Sent choice: %s\n", choice);

            if (!is_valid_choice(choice, "admin")) {
                printf("\nInvalid choice. Choose 1, 2, 3, 4, or 5\n\n");
                continue;
            }

            if (strcmp(choice, "1") == 0) {
                write(client_sock, "add_student", strlen("add_student"));
                AddStudent(client_sock);
            } else if (strcmp(choice, "2") == 0) {
                write(client_sock, "add_faculty", strlen("add_faculty"));
                AddFaculty(client_sock);
            } else if (strcmp(choice, "3") == 0) {
                write(client_sock, "toggle_student_status", strlen("toggle_student_status"));
                ToggleStudentStatus(client_sock);
            } else if (strcmp(choice, "4") == 0) {
                write(client_sock, "update_details", strlen("update_details"));
                UpdateDetails(client_sock);
            } else if (strcmp(choice, "5") == 0) {
                write(client_sock, "exit", strlen("exit"));
                close(client_sock);
                printf("\nExiting Admin Dashboard\n\n");
                exit(0);
            }
        }
    } else if (is_faculty) {
        while (1) {
            printf("\n=== Faculty Dashboard ===\n");
            printf("1) Add new course\n");
            printf("2) Remove offered course\n");
            printf("3) View enrollments in courses\n");
            printf("4) Change password\n");
            printf("5) Exit\n");
            printf("Enter your choice: ");
            scanf("%4s", choice);
            clear_input_buffer();

            if (!is_valid_choice(choice, "faculty")) {
                printf("\nInvalid choice. Choose 1, 2, 3, 4, or 5\n\n");
                continue;
            }

            if (strcmp(choice, "1") == 0) {
                write(client_sock, "add_course", strlen("add_course"));
                AddCourse(client_sock);
            } else if (strcmp(choice, "2") == 0) {
                write(client_sock, "remove_course", strlen("remove_course"));
                RemoveCourse(client_sock);
            } else if (strcmp(choice, "3") == 0) {
                write(client_sock, "view_course_enrollments", strlen("view_course_enrollments"));
                ViewCourseEnrollments(client_sock);
            } else if (strcmp(choice, "4") == 0) {
                write(client_sock, "change_password", strlen("change_password"));
                ChangePassword(client_sock);
            } else if (strcmp(choice, "5") == 0) {
                write(client_sock, "exit", strlen("exit"));
                close(client_sock);
                printf("\nExiting Faculty Dashboard\n\n");
                exit(0);
            }
        }
    } else {
        while (1) {
            printf("\n=== Student Dashboard ===\n");
            printf("1) Enroll in new course\n");
            printf("2) Unenroll from course\n");
            printf("3) View enrolled courses\n");
            printf("4) Change password\n");
            printf("5) Exit\n");
            printf("Enter your choice: ");
            scanf("%4s", choice);
            clear_input_buffer();

            if (!is_valid_choice(choice, "student")) {
                printf("\nInvalid choice. Choose 1, 2, 3, 4, or 5\n\n");
                continue;
            }

            if (strcmp(choice, "1") == 0) {
                write(client_sock, "enroll_course", strlen("enroll_course"));
                EnrollCourse(client_sock);
            } else if (strcmp(choice, "2") == 0) {
                write(client_sock, "unenroll_course", strlen("unenroll_course"));
                UnenrollCourse(client_sock);
            } else if (strcmp(choice, "3") == 0) {
                write(client_sock, "view_enrolled_courses", strlen("view_enrolled_courses"));
                ViewEnrolledCourses(client_sock);
            } else if (strcmp(choice, "4") == 0) {
                write(client_sock, "change_password", strlen("change_password"));
                ChangePassword(client_sock);
            } else if (strcmp(choice, "5") == 0) {
                write(client_sock, "exit", strlen("exit"));
                close(client_sock);
                printf("\nExiting Student Dashboard\n\n");
                exit(0);
            }
        }
    }

    close(client_sock);
    return 0;
}
