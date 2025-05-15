#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>

#define MAX_USERNAME_LENGTH 25
#define MAX_PASSWORD_LENGTH 25
#define MAX_NAME_LENGTH 50
#define MAX_ROLE_LENGTH 10

void RegisterAdmin(int *client_sock, char *data, int bytes_read);
void AddStudent(int *client_sock, char *data, int bytes_read);
void AddFaculty(int *client_sock, char *data, int bytes_read);
void ToggleStudentStatus(int *client_sock, char *data, int bytes_read);
void UpdateDetails(int *client_sock, char *data, int bytes_read);
void EnrollCourse(int *client_sock, char *data, int bytes_read);
void UnenrollCourse(int *client_sock, char *data, int bytes_read);
void ViewEnrolledCourses(int *client_sock, char *data, int bytes_read);
void AddCourse(int *client_sock, char *data, int bytes_read);
void RemoveCourse(int *client_sock, char *data, int bytes_read);
void ViewCourseEnrollments(int *client_sock, char *data, int bytes_read);

int ChangePassword(int *client_sock, char *data, int bytes_read) {
    char username[25], new_password[25];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read new password");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(new_password, data);

    FILE *users = fopen("users.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if (!users || !temp) {
        perror("Cannot open users file");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(users);
    fcntl(fd, F_SETLKW, &lock);

    struct flock lock_temp;
    lock_temp.l_type = F_WRLCK;
    lock_temp.l_whence = SEEK_SET;
    lock_temp.l_start = 0;
    lock_temp.l_len = 0;
    lock_temp.l_pid = getpid();
    int fd_temp = fileno(temp);
    fcntl(fd_temp, F_SETLKW, &lock_temp);

    char temp_user[25], temp_pass[25], temp_role[10];
    int found = 0;
    while (fscanf(users, "%s %s %s", temp_user, temp_pass, temp_role) != EOF) {
        if (strcmp(temp_user, username) == 0) {
            found = 1;
            fprintf(temp, "%s %s %s\n", temp_user, new_password, temp_role);
        } else {
            fprintf(temp, "%s %s %s\n", temp_user, temp_pass, temp_role);
        }
    }

    lock_temp.l_type = F_UNLCK;
    fcntl(fd_temp, F_SETLK, &lock_temp);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(users);
    fclose(temp);

    if (!found) {
        write(*client_sock, "User not found\n", 15);
        remove("temp.txt");
        return 1;
    }

    remove("users.txt");
    rename("temp.txt", "users.txt");
    write(*client_sock, "Password changed\n", 17);
    return 1;
}

// Replace only the Authenticate function in the previous server.c
int Authenticate(int *client_sock, char *data, int bytes_read) {
    char username[25], password[25], role[10];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read username");
        else printf("Client disconnected while reading username\n");
        write(*client_sock, "Login failed\n", 13);
        return 1;
    }
    data[bytes_read] = '\0';
    strcpy(username, data);
    printf("Received username: %s\n", username);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read password");
        else printf("Client disconnected while reading password\n");
        write(*client_sock, "Login failed\n", 13);
        return 1;
    }
    data[bytes_read] = '\0';
    strcpy(password, data);
    printf("Received password: %s\n", password);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 9);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read role");
        else printf("Client disconnected while reading role\n");
        write(*client_sock, "Login failed\n", 13);
        return 1;
    }
    data[bytes_read] = '\0';
    strcpy(role, data);
    printf("Received role: %s\n", role);

    FILE *users = fopen("users.txt", "r");
    if (!users) {
        perror("Cannot open users.txt");
        write(*client_sock, "No users registered\n", 20);
        return 1;
    }
    printf("Opened users.txt\n");

    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(users);

    // Try to acquire lock with a timeout (non-blocking)
    struct timespec timeout;
    timeout.tv_sec = 5; // 5-second timeout
    timeout.tv_nsec = 0;
    lock.l_type = F_RDLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("Failed to acquire read lock on users.txt");
        write(*client_sock, "Server busy, try again\n", 23);
        fclose(users);
        return 1;
    }
    printf("Acquired read lock on users.txt\n");

    char stored_user[25], stored_pass[25], stored_role[10];
    int authenticated = 0;
    while (fscanf(users, "%s %s %s", stored_user, stored_pass, stored_role) != EOF) {
        printf("Checking user: %s, role: %s\n", stored_user, stored_role);
        if (strcmp(stored_user, username) == 0 && strcmp(stored_pass, password) == 0 && strcmp(stored_role, role) == 0) {
            authenticated = 1;
            break;
        }
    }

    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("Failed to release read lock on users.txt");
    }
    printf("Released read lock on users.txt\n");
    fclose(users);
    printf("Closed users.txt\n");

    if (authenticated) {
        char response[50];
        sprintf(response, "%s login successful\n", role);
        write(*client_sock, response, strlen(response));
        printf("Sent response: %s", response);
    } else {
        write(*client_sock, "Login failed\n", 13);
        printf("Sent response: Login failed\n");
    }
    return 1;
}

// Updated handle_client function (main loop only)
void *HandleClient(void *client_sock_ptr) {
    int client_sock = *(int *)client_sock_ptr;
    char data[1024];
    int bytes_read;

    while (1) {
        memset(data, 0, 1024);
        bytes_read = read(client_sock, data, 1023);
        if (bytes_read <= 0) {
            if (bytes_read < 0) perror("Failed to read client data");
            else printf("Client disconnected\n");
            close(client_sock);
            free(client_sock_ptr);
            pthread_exit(NULL);
        }
        data[bytes_read] = '\0';
        printf("Received action: %s\n", data);

        if (strcmp(data, "register_admin") == 0) {
            RegisterAdmin(&client_sock, data, bytes_read);
        } else if (strcmp(data, "login") == 0) {
            Authenticate(&client_sock, data, bytes_read);
        } else if (strcmp(data, "add_student") == 0) {
            AddStudent(&client_sock, data, bytes_read);
        } else if (strcmp(data, "add_faculty") == 0) {
            AddFaculty(&client_sock, data, bytes_read);
        } else if (strcmp(data, "toggle_student_status") == 0) {
            ToggleStudentStatus(&client_sock, data, bytes_read);
        } else if (strcmp(data, "update_details") == 0) {
            UpdateDetails(&client_sock, data, bytes_read);
        } else if (strcmp(data, "enroll_course") == 0) {
            EnrollCourse(&client_sock, data, bytes_read);
        } else if (strcmp(data, "unenroll_course") == 0) {
            UnenrollCourse(&client_sock, data, bytes_read);
        } else if (strcmp(data, "view_enrolled_courses") == 0) {
            ViewEnrolledCourses(&client_sock, data, bytes_read);
        } else if (strcmp(data, "change_password") == 0) {
            ChangePassword(&client_sock, data, bytes_read);
        } else if (strcmp(data, "add_course") == 0) {
            AddCourse(&client_sock, data, bytes_read);
        } else if (strcmp(data, "remove_course") == 0) {
            RemoveCourse(&client_sock, data, bytes_read);
        } else if (strcmp(data, "view_course_enrollments") == 0) {
            ViewCourseEnrollments(&client_sock, data, bytes_read);
        } else if (strcmp(data, "exit") == 0) {
            printf("Client requested exit\n");
            close(client_sock);
            free(client_sock_ptr);
            pthread_exit(NULL);
        } else {
            write(client_sock, "Invalid action\n", 15);
            printf("Sent response: Invalid action\n");
        }
    }
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    int server_sock, client_sock;
    socklen_t client_len;

    printf("Course management server starting\n");

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9050);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_sock, 50) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server running on port %d\n", ntohs(server_addr.sin_port));

    while (1) {
        client_len = sizeof(client_addr);
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (*client_sock_ptr < 0) {
            perror("Accept failed");
            free(client_sock_ptr);
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, HandleClient, (void *)client_sock_ptr);
        pthread_detach(thread_id);
    }

    close(server_sock);
    return 0;
}
