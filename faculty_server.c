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

int AddCourse(int *client_sock, char *data, int bytes_read) {
    char username[25], course_id[10], course_name[50], seats_str[10];
    int seats;
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 9);
    if (bytes_read < 0) {
        perror("Failed to read course ID");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(course_id, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 49);
    if (bytes_read < 0) {
        perror("Failed to read course name");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(course_name, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 9);
    if (bytes_read < 0) {
        perror("Failed to read seats");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(seats_str, data);
    seats = atoi(seats_str);

    FILE *courses = fopen("courses.txt", "a+");
    if (!courses) {
        perror("Cannot open courses file");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(courses);
    fcntl(fd, F_SETLKW, &lock);

    char temp_id[10];
    int exists = 0;
    rewind(courses);
    while (fscanf(courses, "%s %*s %*s %*d %*d", temp_id) != EOF) {
        if (strcmp(temp_id, course_id) == 0) {
            exists = 1;
            break;
        }
    }

    if (exists) {
        write(*client_sock, "Course ID already exists\n", 25);
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        fclose(courses);
        return 1;
    }

    fprintf(courses, "%s %s %s %d %d\n", course_id, course_name, username, seats, seats);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(courses);

    write(*client_sock, "Course added\n\n", 13);
    printf("Course %s added by %s\n", course_id, username);
    return 1;
}

int RemoveCourse(int *client_sock, char *data, int bytes_read) {
    char username[25], course_id[10];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 9);
    if (bytes_read < 0) {
        perror("Failed to read course ID");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(course_id, data);

    FILE *courses = fopen("courses.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if (!courses || !temp) {
        perror("Cannot open course files");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(courses);
    fcntl(fd, F_SETLKW, &lock);

    struct flock lock_temp;
    lock_temp.l_type = F_WRLCK;
    lock_temp.l_whence = SEEK_SET;
    lock_temp.l_start = 0;
    lock_temp.l_len = 0;
    lock_temp.l_pid = getpid();
    int fd_temp = fileno(temp);
    fcntl(fd_temp, F_SETLKW, &lock_temp);

    char temp_id[10], temp_name[50], temp_faculty[25];
    int temp_seats, temp_available, found = 0;
    while (fscanf(courses, "%s %s %s %d %d", temp_id, temp_name, temp_faculty, &temp_seats, &temp_available) != EOF) {
        if (strcmp(temp_id, course_id) == 0 && strcmp(temp_faculty, username) == 0) {
            found = 1;
            continue;
        }
        fprintf(temp, "%s %s %s %d %d\n", temp_id, temp_name, temp_faculty, temp_seats, temp_available);
    }

    lock_temp.l_type = F_UNLCK;
    fcntl(fd_temp, F_SETLK, &lock_temp);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(courses);
    fclose(temp);

    if (!found) {
        write(*client_sock, "Course not found or not owned\n", 30);
        remove("temp.txt");
        return 1;
    }

    remove("courses.txt");
    rename("temp.txt", "courses.txt");

    FILE *enrollments = fopen("enrollments.txt", "r");
    FILE *temp_enroll = fopen("temp_enroll.txt", "w");
    if (!enrollments || !temp_enroll) {
        perror("Cannot open enrollment files");
        exit(EXIT_FAILURE);
    }

    lock.l_type = F_WRLCK;
    fd = fileno(enrollments);
    fcntl(fd, F_SETLKW, &lock);

    lock_temp.l_type = F_WRLCK;
    fd_temp = fileno(temp_enroll);
    fcntl(fd_temp, F_SETLKW, &lock_temp);

    char temp_user[25], temp_course[10];
    while (fscanf(enrollments, "%s %s", temp_user, temp_course) != EOF) {
        if (strcmp(temp_course, course_id) != 0) {
            fprintf(temp_enroll, "%s %s\n", temp_user, temp_course);
        }
    }

    lock_temp.l_type = F_UNLCK;
    fcntl(fd_temp, F_SETLK, &lock_temp);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(enrollments);
    fclose(temp_enroll);
    remove("enrollments.txt");
    rename("temp_enroll.txt", "enrollments.txt");

    write(*client_sock, "Course removed\n", 15);
    return 1;
}

int ViewCourseEnrollments(int *client_sock, char *data, int bytes_read) {
    char username[25];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    FILE *courses = fopen("courses.txt", "r");
    if (!courses) {
        perror("Cannot open courses file");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(courses);
    fcntl(fd, F_SETLKW, &lock);

    FILE *enrollments = fopen("enrollments.txt", "r");
    if (!enrollments) {
        perror("Cannot open enrollments file");
        exit(EXIT_FAILURE);
    }

    char course_id[10], course_name[50], faculty[25];
    int seats, available, found = 0;
    while (fscanf(courses, "%s %s %s %d %d", course_id, course_name, faculty, &seats, &available) != EOF) {
        if (strcmp(faculty, username) == 0) {
            found = 1;
            char line[100];
            sprintf(line, "Course: %s - %s\nStudents:\n", course_id, course_name);
            write(*client_sock, line, strlen(line));

            rewind(enrollments);
            char temp_user[25], temp_course[10];
            int has_students = 0;
            while (fscanf(enrollments, "%s %s", temp_user, temp_course) != EOF) {
                if (strcmp(temp_course, course_id) == 0) {
                    has_students = 1;
                    char student_line[50];
                    sprintf(student_line, "%s\n", temp_user);
                    write(*client_sock, student_line, strlen(student_line));
                }
            }
            if (!has_students) {
                write(*client_sock, "No students enrolled\n", 20);
            }
            write(*client_sock, "---\n", 4);
        }
    }

    if (!found) {
        write(*client_sock, "No courses found\n", 17);
    }

    write(*client_sock, "End of list\n", 12);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(courses);
    fclose(enrollments);
    return 1;
}