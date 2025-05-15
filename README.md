# Course Management System (Client-Server Model)

This system is a C-based client-server application designed to manage a course registration system for three types of users: **Admin**, **Faculty**, and **Students**. It uses TCP sockets for communication and file locking to manage data consistency.

---

## Contents

* [System Overview](#system-overview)
* [File Structure](#file-structure)
* [Compilation and Execution](#compilation-and-execution)
* [Code Flow](#code-flow)
* [Function Descriptions](#function-descriptions)

---

## System Overview

* **Client Types**: Admin, Faculty, and Student clients each have their own interfaces.
* **Server Side**: Handles client requests and accesses files like `courses.txt`, `students.txt`, `faculties.txt`, and `enrollments.txt`.
* **Communication**: Clients communicate with the server using TCP sockets.
* **Concurrency**: `pthread` is used to handle multiple clients concurrently.

---

## File Structure

* `main_client.c` - Entry point for the client. Delegates to respective client modules.
* `admin_client.c` - Admin-specific interface and communication logic.
* `faculty_client.c` - Faculty-specific interface and communication logic.
* `student_client.c` - Student-specific interface and communication logic.
* `client_utils.c` - Shared utility functions for clients (e.g., input reading).
* `main_server.c` - Accepts connections, identifies roles, and routes requests.
* `admin_server.c` - Handles admin-related functionalities on the server side.
* `faculty_server.c` - Handles faculty-related functionalities on the server side.
* `student_server.c` - Handles student-related functionalities on the server side.

---

## Compilation and Execution

### Compile

```bash
gcc main_client.c client_utils.c -o client -lpthread
gcc main_server.c admin_server.c faculty_server.c student_server.c -o server -lpthread
```

### Run Server

```bash
./server
```

### Run Client

```bash
./client
```

---

## Code Flow

### Client Side

1. `main_client.c` launches the CLI and prompts for user type (Admin/Faculty/Student).
2. Based on the user type, it calls the appropriate module (`admin_client.c`, etc.).
3. Sends commands/data to the server via TCP.
4. Waits for and displays server responses.

### Server Side

1. `main_server.c` listens for incoming connections.
2. On connection, creates a thread to handle each client.
3. Based on user role, routes the client to `admin_server.c`, `faculty_server.c`, or `student_server.c`.
4. Executes appropriate function, accessing/modifying files with file-locking for concurrency safety.

---

## Function Descriptions

### Student Server (`student_server.c`)

* **EnrollCourse**

  * Reads username and course ID.
  * Validates student status and course availability.
  * Locks files and updates `courses.txt` (seat count) and `enrollments.txt`.
  * Sends success or error messages to client.

* **UnenrollCourse**

  * Reads username and course ID.
  * Removes matching entry from `enrollments.txt`.
  * Increments course availability in `courses.txt`.

* **ViewEnrolledCourses**

  * Reads username.
  * Looks up enrollments in `enrollments.txt`.
  * For each enrolled course, fetches course details from `courses.txt` and sends back to client.

### Faculty Server (`faculty_server.c`)

* **AddCourse**

  * Faculty adds a course with course ID, name, seat capacity.
  * Adds entry to `courses.txt`.

* **ViewCourses**

  * Shows all courses created by the faculty.

* **ViewEnrolledStudents**

  * Shows list of students enrolled in the faculty's courses by cross-referencing `courses.txt` and `enrollments.txt`.

### Admin Server (`admin_server.c`)

* **AddUser**

  * Adds new users (Student/Faculty) with status `active`.
  * Appends to `students.txt` or `faculties.txt` based on role.

* **Block/Unblock User**

  * Modifies user status (active/inactive) in respective files.

* **RemoveUser**

  * Removes user by rewriting file without the specified user.

### Utility (`client_utils.c`)

* Provides helper functions for reading user input, clearing input buffers, and standardized printing.

---

## File Locking

To avoid race conditions in file access:

* `fcntl` file locks are used for both read and write operations.
* Each server function locks the file before access and unlocks after modification.

---

## Data Files Format

* **students.txt**: `username full_name status`
* **faculties.txt**: `username full_name status`
* **courses.txt**: `course_id name faculty seats available`
* **enrollments.txt**: `username course_id`

---

## Enhancements Suggested

* Replace text files with database for better concurrency.
* Encrypt communication.
* Add authentication (username/password based).
* Introduce logs for auditing actions.

---

This system serves as a comprehensive CLI-based learning tool to understand client-server architecture, file handling with concurrency, and structured multi-user interaction in C.
