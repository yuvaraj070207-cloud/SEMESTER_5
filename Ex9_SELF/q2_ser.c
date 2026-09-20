// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080
#define NUM_SUBJECTS 5
#define DB_FILE "students.dat"
#define LOG_FILE "transactions.log"

// Commands
typedef enum {
    CMD_ADD = 1,
    CMD_SEARCH,
    CMD_UPDATE,
    CMD_DELETE,
    CMD_DISPLAY_ALL,
    CMD_TOPPER
} CommandType;

// Student Structure
typedef struct {
    int reg_no;
    char name[50];
    char dept[20];
    int semester;
    float marks[NUM_SUBJECTS];
    char grade[5];
    float cgpa;
} Student;

// Request Payload
typedef struct {
    CommandType cmd;
    int reg_no;
    Student student_data;
} Request;

// Response Payload
typedef struct {
    int success;
    char message[256];
    int count;
    Student students[50];
} Response;

// Grade and CGPA Calculator
void calculate_grade_and_cgpa(Student *s) {
    float total = 0;
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        total += s->marks[i];
    }
    float avg = total / NUM_SUBJECTS;
    s->cgpa = avg / 10.0f;

    if (s->cgpa >= 9.0) strcpy(s->grade, "O");
    else if (s->cgpa >= 8.0) strcpy(s->grade, "A+");
    else if (s->cgpa >= 7.0) strcpy(s->grade, "A");
    else if (s->cgpa >= 6.0) strcpy(s->grade, "B+");
    else if (s->cgpa >= 5.0) strcpy(s->grade, "B");
    else strcpy(s->grade, "F");
}

// Transaction Logger
void log_transaction(const char *client_ip, const char *action) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strcspn(time_str, "\n")] = 0;

    fprintf(fp, "[%s] Client [%s]: %s\n", time_str, client_ip, action);
    fclose(fp);
}

// Database Handlers
Response handle_add(Student s) {
    Response res = {0};
    FILE *fp = fopen(DB_FILE, "rb+");
    if (!fp) fp = fopen(DB_FILE, "wb+");

    Student temp;
    while (fread(&temp, sizeof(Student), 1, fp)) {
        if (temp.reg_no == s.reg_no) {
            res.success = 0;
            strcpy(res.message, "Error: Student with this Register Number already exists.");
            fclose(fp);
            return res;
        }
    }

    calculate_grade_and_cgpa(&s);
    fseek(fp, 0, SEEK_END);
    fwrite(&s, sizeof(Student), 1, fp);
    fclose(fp);

    res.success = 1;
    strcpy(res.message, "Student record added successfully.");
    return res;
}

Response handle_search(int reg_no) {
    Response res = {0};
    FILE *fp = fopen(DB_FILE, "rb");
    if (!fp) {
        res.success = 0;
        strcpy(res.message, "Error: Database file missing or empty.");
        return res;
    }

    Student temp;
    while (fread(&temp, sizeof(Student), 1, fp)) {
        if (temp.reg_no == reg_no) {
            res.success = 1;
            res.count = 1;
            res.students[0] = temp;
            strcpy(res.message, "Record found successfully.");
            fclose(fp);
            return res;
        }
    }

    fclose(fp);
    res.success = 0;
    strcpy(res.message, "Error: Register Number not found.");
    return res;
}

Response handle_update(Student s) {
    Response res = {0};
    FILE *fp = fopen(DB_FILE, "rb+");
    if (!fp) {
        res.success = 0;
        strcpy(res.message, "Error: Database file missing.");
        return res;
    }

    Student temp;
    int found = 0;
    while (fread(&temp, sizeof(Student), 1, fp)) {
        if (temp.reg_no == s.reg_no) {
            found = 1;
            calculate_grade_and_cgpa(&s);
            fseek(fp, -sizeof(Student), SEEK_CUR);
            fwrite(&s, sizeof(Student), 1, fp);
            break;
        }
    }
    fclose(fp);

    if (found) {
        res.success = 1;
        strcpy(res.message, "Student record updated successfully.");
    } else {
        res.success = 0;
        strcpy(res.message, "Error: Student record not found for update.");
    }
    return res;
}

Response handle_delete(int reg_no) {
    Response res = {0};
    FILE *fp = fopen(DB_FILE, "rb");
    if (!fp) {
        res.success = 0;
        strcpy(res.message, "Error: Database file missing.");
        return res;
    }

    FILE *temp_fp = fopen("temp.dat", "wb");
    Student temp;
    int found = 0;

    while (fread(&temp, sizeof(Student), 1, fp)) {
        if (temp.reg_no == reg_no) {
            found = 1;
        } else {
            fwrite(&temp, sizeof(Student), 1, temp_fp);
        }
    }
    fclose(fp);
    fclose(temp_fp);

    remove(DB_FILE);
    rename("temp.dat", DB_FILE);

    if (found) {
        res.success = 1;
        strcpy(res.message, "Student record deleted successfully.");
    } else {
        res.success = 0;
        strcpy(res.message, "Error: Register Number not found.");
    }
    return res;
}

Response handle_display_all() {
    Response res = {0};
    FILE *fp = fopen(DB_FILE, "rb");
    if (!fp) {
        res.success = 0;
        strcpy(res.message, "Error: Database file missing.");
        return res;
    }

    res.count = 0;
    while (fread(&res.students[res.count], sizeof(Student), 1, fp)) {
        res.count++;
    }
    fclose(fp);

    res.success = 1;
    snprintf(res.message, sizeof(res.message), "Retrieved %d student records.", res.count);
    return res;
}

Response handle_topper() {
    Response res = {0};
    FILE *fp = fopen(DB_FILE, "rb");
    if (!fp) {
        res.success = 0;
        strcpy(res.message, "Error: Database empty or missing.");
        return res;
    }

    Student temp, topper;
    int found = 0;
    float max_cgpa = -1.0f;

    while (fread(&temp, sizeof(Student), 1, fp)) {
        if (temp.cgpa > max_cgpa) {
            max_cgpa = temp.cgpa;
            topper = temp;
            found = 1;
        }
    }
    fclose(fp);

    if (found) {
        res.success = 1;
        res.count = 1;
        res.students[0] = topper;
        strcpy(res.message, "Class topper details retrieved.");
    } else {
        res.success = 0;
        strcpy(res.message, "Error: No records available.");
    }
    return res;
}

void sigchld_handler(int s) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    signal(SIGCHLD, sigchld_handler);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Result Management Server listening on port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) continue;

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);

        if (fork() == 0) { // Child Process
            close(server_fd);

            Request req;
            Response res;

            while (read(new_socket, &req, sizeof(Request)) > 0) {
                char log_msg[256];

                switch (req.cmd) {
                    case CMD_ADD:
                        snprintf(log_msg, sizeof(log_msg), "CMD_ADD (RegNo: %d)", req.student_data.reg_no);
                        res = handle_add(req.student_data);
                        break;
                    case CMD_SEARCH:
                        snprintf(log_msg, sizeof(log_msg), "CMD_SEARCH (RegNo: %d)", req.reg_no);
                        res = handle_search(req.reg_no);
                        break;
                    case CMD_UPDATE:
                        snprintf(log_msg, sizeof(log_msg), "CMD_UPDATE (RegNo: %d)", req.student_data.reg_no);
                        res = handle_update(req.student_data);
                        break;
                    case CMD_DELETE:
                        snprintf(log_msg, sizeof(log_msg), "CMD_DELETE (RegNo: %d)", req.reg_no);
                        res = handle_delete(req.reg_no);
                        break;
                    case CMD_DISPLAY_ALL:
                        snprintf(log_msg, sizeof(log_msg), "CMD_DISPLAY_ALL");
                        res = handle_display_all();
                        break;
                    case CMD_TOPPER:
                        snprintf(log_msg, sizeof(log_msg), "CMD_TOPPER");
                        res = handle_topper();
                        break;
                    default:
                        snprintf(log_msg, sizeof(log_msg), "INVALID_COMMAND");
                        res.success = 0;
                        strcpy(res.message, "Error: Invalid operation.");
                }

                log_transaction(client_ip, log_msg);
                write(new_socket, &res, sizeof(Response));
            }

            close(new_socket);
            exit(0);
        }

        close(new_socket); // Parent Process
    }

    return 0;
}