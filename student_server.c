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

int EnrollCourse(int *client_sock, char *data, int bytes_read) {
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

    FILE *students = fopen("students.txt", "r");
    if (!students) {
        perror("Cannot open students file");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(students);
    fcntl(fd, F_SETLKW, &lock);

    char temp_user[25], temp_name[50], temp_status[10];
    int is_active = 0;
    while (fscanf(students, "%s %s %s", temp_user, temp_name, temp_status) != EOF) {
        if (strcmp(temp_user, username) == 0 && strcmp(temp_status, "active") == 0) {
            is_active = 1;
            break;
        }
    }

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(students);

    if (!is_active) {
        write(*client_sock, "Student not active\n\n", 19);
        return 1;
    }

    FILE *courses = fopen("courses.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if (!courses || !temp) {
        perror("Cannot open course files");
        exit(EXIT_FAILURE);
    }

    lock.l_type = F_WRLCK;
    fd = fileno(courses);
    fcntl(fd, F_SETLKW, &lock);

    struct flock lock_temp;
    lock_temp.l_type = F_WRLCK;
    lock_temp.l_whence = SEEK_SET;
    lock_temp.l_start = 0;
    lock_temp.l_len = 0;
    lock_temp.l_pid = getpid();
    int fd_temp = fileno(temp);
    fcntl(fd_temp, F_SETLKW, &lock_temp);

    char temp_id[10], course_name[50], temp_faculty[25];
    int temp_seats, temp_available, found = 0;
    while (fscanf(courses, "%s %s %s %d %d", temp_id, course_name, temp_faculty, &temp_seats, &temp_available) != EOF) {
        if (strcmp(temp_id, course_id) == 0) {
            found = 1;
            if (temp_available <= 0) {
                write(*client_sock, "No seats available\n", 20);
                fprintf(temp, "%s %s %s %d %d\n", temp_id, course_name, temp_faculty, temp_seats, temp_available);
                lock_temp.l_type = F_UNLCK;
                fcntl(fd_temp, F_SETLK, &lock_temp);
                lock.l_type = F_UNLCK;
                fcntl(fd, F_SETLK, &lock);
                fclose(courses);
                fclose(temp);
                remove("temp.txt");
                return 1;
            }
            temp_available--;
            fprintf(temp, "%s %s %s %d %d\n", temp_id, course_name, temp_faculty, temp_seats, temp_available);
        } else {
            fprintf(temp, "%s %s %s %d %d\n", temp_id, course_name, temp_faculty, temp_seats, temp_available);
        }
    }

    lock_temp.l_type = F_UNLCK;
    fcntl(fd_temp, F_SETLK, &lock_temp);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(courses);
    fclose(temp);

    if (!found) {
        write(*client_sock, "Course not found\n", 17);
        remove("temp.txt");
        return 1;
    }

    remove("courses.txt");
    rename("temp.txt", "courses.txt");

    FILE *enrollments = fopen("enrollments.txt", "a");
    if (!enrollments) {
        perror("Cannot open enrollments file");
        exit(EXIT_FAILURE);
    }

    lock.l_type = F_WRLCK;
    fd = fileno(enrollments);
    fcntl(fd, F_SETLKW, &lock);

    fprintf(enrollments, "%s %s\n", username, course_id);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(enrollments);

    write(*client_sock, "Enrolled in course\n\n", 19);
    return 1;
}

int UnenrollCourse(int *client_sock, char *data, int bytes_read) {
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

    FILE *enrollments = fopen("enrollments.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if (!enrollments || !temp) {
        perror("Cannot open enrollment files");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(enrollments);
    fcntl(fd, F_SETLKW, &lock);

    struct flock lock_temp;
    lock_temp.l_type = F_WRLCK;
    lock_temp.l_whence = SEEK_SET;
    lock_temp.l_start = 0;
    lock_temp.l_len = 0;
    lock_temp.l_pid = getpid();
    int fd_temp = fileno(temp);
    fcntl(fd_temp, F_SETLKW, &lock_temp);

    char temp_user[25], temp_course[10];
    int found = 0;
    while (fscanf(enrollments, "%s %s", temp_user, temp_course) != EOF) {
        if (strcmp(temp_user, username) == 0 && strcmp(temp_course, course_id) == 0) {
            found = 1;
            continue;
        }
        fprintf(temp, "%s %s\n", temp_user, temp_course);
    }

    lock_temp.l_type = F_UNLCK;
    fcntl(fd_temp, F_SETLK, &lock_temp);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(enrollments);
    fclose(temp);

    if (!found) {
        write(*client_sock, "Enrollment not found\n", 21);
        remove("temp.txt");
        return 1;
    }

    remove("enrollments.txt");
    rename("temp.txt", "enrollments.txt");

    FILE *courses = fopen("courses.txt", "r");
    FILE *temp_courses = fopen("temp_courses.txt", "w");
    if (!courses || !temp_courses) {
        perror("Cannot open course files");
        exit(EXIT_FAILURE);
    }

    lock.l_type = F_WRLCK;
    fd = fileno(courses);
    fcntl(fd, F_SETLKW, &lock);

    lock_temp.l_type = F_WRLCK;
    fd_temp = fileno(temp_courses);
    fcntl(fd_temp, F_SETLKW, &lock_temp);

    char temp_id[10], temp_name[50], temp_faculty[25];
    int temp_seats, temp_available;
    while (fscanf(courses, "%s %s %s %d %d", temp_id, temp_name, temp_faculty, &temp_seats, &temp_available) != EOF) {
        if (strcmp(temp_id, course_id) == 0) {
            temp_available++;
            fprintf(temp_courses, "%s %s %s %d %d\n", temp_id, temp_name, temp_faculty, temp_seats, temp_available);
        } else {
            fprintf(temp_courses, "%s %s %s %d %d\n", temp_id, temp_name, temp_faculty, temp_seats, temp_available);
        }
    }

    lock_temp.l_type = F_UNLCK;
    fcntl(fd_temp, F_SETLK, &lock_temp);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(courses);
    fclose(temp_courses);
    remove("courses.txt");
    rename("temp_courses.txt", "courses.txt");

    write(*client_sock, "Unenrolled from course\n\n", 23);
    return 1;
}

int ViewEnrolledCourses(int *client_sock, char *data, int bytes_read) {
    char username[25];
    memset(data, 0, sizeof(data));
    bytes_read = read(*client_sock, data, 24);
    if (bytes_read < 0) {
        perror("Failed to read username");
        exit(1);
    }
    data[bytes_read] = '\0';
    strcpy(username, data);

    FILE *enrollments = fopen("enrollments.txt", "r");
    if (!enrollments) {
        perror("Cannot open enrollments file");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    int fd = fileno(enrollments);
    fcntl(fd, F_SETLKW, &lock);

    FILE *courses = fopen("courses.txt", "r");
    if (!courses) {
        perror("Cannot open courses file");
        exit(EXIT_FAILURE);
    }

    char temp_user[25], temp_course[10];
    int found = 0;
    while (fscanf(enrollments, "%s %s", temp_user, temp_course) != EOF) {
        if (strcmp(temp_user, username) == 0) {
            found = 1;
            rewind(courses);
            char course_id[10], course_name[50], faculty[25];
            int seats, available;
            while (fscanf(courses, "%s %s %s %d %d", course_id, course_name, faculty, &seats, &available) != EOF) {
                if (strcmp(course_id, temp_course) == 0) {
                    char line[100];
                    sprintf(line, "%s - %s\n", course_id, course_name);
                    write(*client_sock, line, strlen(line));
                    break;
                }
            }
        }
    }

    if (!found) {
        write(*client_sock, "No courses enrolled\n", 20);
    }

    write(*client_sock, "End of list\n", 12);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(enrollments);
    fclose(courses);
    return 1;
}