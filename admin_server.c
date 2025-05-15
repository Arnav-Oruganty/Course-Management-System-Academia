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

// Function to register an admin
int RegisterAdmin(int *client_sock, char *data, int bytes_read) {
    char username[MAX_USERNAME_LENGTH], password[MAX_PASSWORD_LENGTH], name[MAX_NAME_LENGTH];
    memset(data, 0, 1024);
    bytes_read = read(*client_sock, data, MAX_USERNAME_LENGTH - 1);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read username");
        else printf("Client disconnected while reading username\n");
        write(*client_sock, "Registration failed\n", 20);
        return 1;
    }
    data[bytes_read] = '\0';
    strcpy(username, data);
    printf("Received username: %s\n", username);

    memset(data, 0, 1024);
    bytes_read = read(*client_sock, data, MAX_PASSWORD_LENGTH - 1);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read password");
        else printf("Client disconnected while reading password\n");
        write(*client_sock, "Registration failed\n", 20);
        return 1;
    }
    data[bytes_read] = '\0';
    strcpy(password, data);
    printf("Received password: %s\n", password);

    memset(data, 0, 1024);
    bytes_read = read(*client_sock, data, MAX_NAME_LENGTH - 1);
    if (bytes_read <= 0) {
        if (bytes_read < 0) perror("Failed to read name");
        else printf("Client disconnected while reading name\n");
        write(*client_sock, "Registration failed\n", 20);
        return 1;
    }
    data[bytes_read] = '\0';
    strcpy(name, data);
    printf("Received name: %s\n", name);

    FILE *users = fopen("users.txt", "a+");
    if (!users) {
        perror("Cannot open users.txt");
        write(*client_sock, "Registration failed\n", 20);
        return 1;
    }
    printf("Opened users.txt\n");

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(users);

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("Failed to acquire write lock on users.txt");
        write(*client_sock, "Server busy, try again\n", 23);
        fclose(users);
        return 1;
    }
    printf("Acquired write lock on users.txt\n");

    rewind(users);
    char stored_user[MAX_USERNAME_LENGTH];
    int username_exists = 0;
    char stored_pass[MAX_PASSWORD_LENGTH], stored_role[MAX_ROLE_LENGTH];
    while (fscanf(users, "%s %s %s", stored_user, stored_pass, stored_role) != EOF) {
        if (strcmp(stored_user, username) == 0) {
            username_exists = 1;
            break;
        }
    }

    if (username_exists) {
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        fclose(users);
        write(*client_sock, "Username already exists\n", 24);
        printf("Username %s already exists\n", username);
        return 1;
    }

    fprintf(users, "%s %s admin\n", username, password);
    printf("Registered admin: %s\n", username);

    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("Failed to release write lock on users.txt");
    }
    printf("Released write lock on users.txt\n");
    fclose(users);
    printf("Closed users.txt\n");

    write(*client_sock, "Registration successful\n", 24);
    printf("Sent response: Registration successful\n");
    return 1;
}

int AddStudent(int *client_sock, char *data, int bytes_read) {
    char username[25], name[50], password[25];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 49);
    if (bytes_read < 0) {
        perror("Failed to read name");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(name, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read password");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(password, data);

    FILE *students = fopen("students.txt", "a+");
    if (!students) {
        perror("Cannot open students file");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(students);
    fcntl(fd, F_SETLKW, &lock);

    char temp[25];
    int exists = 0;
    rewind(students);
    while (fscanf(students, "%s %*s %*s", temp) != EOF) {
        if (strcmp(temp, username) == 0) {
            exists = 1;
            break;
        }
    }

    if (exists) {
        write(*client_sock, "Student already exists\n", 22);
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        fclose(students);
        return 1;
    }

    fprintf(students, "%s %s active\n", username, name);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(students);

    FILE *users = fopen("users.txt", "a");
    if (!users) {
        perror("Cannot open users file");
        exit(EXIT_FAILURE);
    }

    lock.l_type = F_WRLCK;
    fd = fileno(users);
    fcntl(fd, F_SETLKW, &lock);

    fprintf(users, "%s %s student\n", username, password);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(users);

    write(*client_sock, "Student added\n\n", 14);
    printf("Added student %s\n", username);
    return 1;
}

int AddFaculty(int *client_sock, char *data, int bytes_read) {
    char username[25], name[50], password[25];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 49);
    if (bytes_read < 0) {
        perror("Failed to read name");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(name, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read password");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(password, data);

    FILE *faculty = fopen("faculty.txt", "a+");
    if (!faculty) {
        perror("Cannot open faculty file");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(faculty);
    fcntl(fd, F_SETLKW, &lock);

    char temp[25];
    int exists = 0;
    rewind(faculty);
    while (fscanf(faculty, "%s %*s", temp) != EOF) {
        if (strcmp(temp, username) == 0) {
            exists = 1;
            break;
        }
    }

    if (exists) {
        write(*client_sock, "Faculty already exists\n", 23);
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        fclose(faculty);
        return 1;
    }

    fprintf(faculty, "%s %s\n", username, name);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(faculty);

    FILE *users = fopen("users.txt", "a");
    if (!users) {
        perror("Cannot open users file");
        exit(EXIT_FAILURE);
    }

    lock.l_type = F_WRLCK;
    fd = fileno(users);
    fcntl(fd, F_SETLKW, &lock);

    fprintf(users, "%s %s faculty\n", username, password);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(users);

    write(*client_sock, "Faculty added\n", 14);
    printf("Added faculty %s\n", username);
    return 1;
}

int ToggleStudentStatus(int *client_sock, char *data, int bytes_read) {
    char username[25];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    FILE *students = fopen("students.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if (!students || !temp) {
        perror("Cannot open student files");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(students);
    fcntl(fd, F_SETLKW, &lock);

    struct flock lock_temp;
    lock_temp.l_type = F_WRLCK;
    lock_temp.l_whence = SEEK_SET;
    lock_temp.l_start = 0;
    lock_temp.l_len = 0;
    lock_temp.l_pid = getpid();
    int fd_temp = fileno(temp);
    fcntl(fd_temp, F_SETLKW, &lock_temp);

    char temp_user[25], temp_name[50], temp_status[10];
    int found = 0;
    while (fscanf(students, "%s %s %s", temp_user, temp_name, temp_status) != EOF) {
        if (strcmp(temp_user, username) == 0) {
            found = 1;
            fprintf(temp, "%s %s %s\n", temp_user, temp_name, strcmp(temp_status, "active") == 0 ? "inactive" : "active");
        } else {
            fprintf(temp, "%s %s %s\n", temp_user, temp_name, temp_status);
        }
    }

    lock_temp.l_type = F_UNLCK;
    fcntl(fd_temp, F_SETLK, &lock_temp);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(students);
    fclose(temp);
    remove("students.txt");
    rename("temp.txt", "students.txt");

    if (found)
        write(*client_sock, "Student status toggled\n\n", 22);
    else
        write(*client_sock, "Student not found\n", 18);
    return 1;
}

int UpdateDetails(int *client_sock, char *data, int bytes_read) {
    char role[10], username[25], new_name[50];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 9);
    if (bytes_read < 0) {
        perror("Failed to read role");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(role, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 49);
    if (bytes_read < 0) {
        perror("Failed to read new name");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(new_name, data);

    char *filename = strcmp(role, "student") == 0 ? "students.txt" : "faculty.txt";
    FILE *file = fopen(filename, "r");
    FILE *temp = fopen("temp.txt", "w");
    if (!file || !temp) {
        perror("Cannot open files");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(file);
    fcntl(fd, F_SETLKW, &lock);

    struct flock lock_temp;
    lock_temp.l_type = F_WRLCK;
    lock_temp.l_whence = SEEK_SET;
    lock_temp.l_start = 0;
    lock_temp.l_len = 0;
    lock_temp.l_pid = getpid();
    int fd_temp = fileno(temp);
    fcntl(fd_temp, F_SETLKW, &lock_temp);

    int found = 0;
    if (strcmp(role, "student") == 0) {
        char temp_user[25], temp_name[50], temp_status[10];
        while (fscanf(file, "%s %s %s", temp_user, temp_name, temp_status) != EOF) {
            if (strcmp(temp_user, username) == 0) {
                found = 1;
                fprintf(temp, "%s %s %s\n", temp_user, new_name, temp_status);
            } else {
                fprintf(temp, "%s %s %s\n", temp_user, temp_name, temp_status);
            }
        }
    } else {
        char temp_user[25], temp_name[50];
        while (fscanf(file, "%s %s", temp_user, temp_name) != EOF) {
            if (strcmp(temp_user, username) == 0) {
                found = 1;
                fprintf(temp, "%s %s\n", temp_user, new_name);
            } else {
                fprintf(temp, "%s %s\n", temp_user, temp_name);
            }
        }
    }

    lock_temp.l_type = F_UNLCK;
    fcntl(fd_temp, F_SETLK, &lock_temp);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(file);
    fclose(temp);
    remove(filename);
    rename("temp.txt", filename);

    if (found)
        write(*client_sock, "Details updated\n", 16);
    else
        write(*client_sock, "User not found\n", 15);
    return 1;
}