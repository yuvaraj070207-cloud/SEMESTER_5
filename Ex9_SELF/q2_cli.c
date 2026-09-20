// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define NUM_SUBJECTS 5

typedef enum {
    CMD_ADD = 1,
    CMD_SEARCH,
    CMD_UPDATE,
    CMD_DELETE,
    CMD_DISPLAY_ALL,
    CMD_TOPPER
} CommandType;

typedef struct {
    int reg_no;
    char name[50];
    char dept[20];
    int semester;
    float marks[NUM_SUBJECTS];
    char grade[5];
    float cgpa;
} Student;

typedef struct {
    CommandType cmd;
    int reg_no;
    Student student_data;
} Request;

typedef struct {
    int success;
    char message[256];
    int count;
    Student students[50];
} Response;

void print_student(Student s) {
    printf("\n----------------------------------------\n");
    printf("Register No : %d\n", s.reg_no);
    printf("Name        : %s\n", s.name);
    printf("Department  : %s\n", s.dept);
    printf("Semester    : %d\n", s.semester);
    printf("Marks       : ");
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        printf("%.1f ", s.marks[i]);
    }
    printf("\nCGPA        : %.2f\n", s.cgpa);
    printf("Grade       : %s\n", s.grade);
    printf("----------------------------------------\n");
}

void input_student_details(Student *s) {
    printf("Enter Register Number: ");
    scanf("%d", &s->reg_no);
    printf("Enter Name: ");
    scanf(" %[^\n]", s->name);
    printf("Enter Department: ");
    scanf(" %[^\n]", s->dept);
    printf("Enter Semester: ");
    scanf("%d", &s->semester);
    printf("Enter Marks for %d Subjects:\n", NUM_SUBJECTS);
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        printf("  Subject %d: ", i + 1);
        scanf("%f", &s->marks[i]);
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed!\n");
        return -1;
    }

    printf("Connected to Student Result Management Server.\n");

    int choice;
    Request req;
    Response res;

    while (1) {
        printf("\n========================================\n");
        printf("  STUDENT RESULT MANAGEMENT SYSTEM\n");
        printf("========================================\n");
        printf("1. Add Student Record\n");
        printf("2. Search Student Record\n");
        printf("3. Update Student Record\n");
        printf("4. Delete Student Record\n");
        printf("5. Display All Records\n");
        printf("6. Generate Class Topper\n");
        printf("7. Exit\n");
        printf("Enter Choice (1-7): ");
        scanf("%d", &choice);

        if (choice == 7) break;

        memset(&req, 0, sizeof(Request));
        req.cmd = (CommandType)choice;

        switch (choice) {
            case 1:
                input_student_details(&req.student_data);
                break;
            case 2:
            case 4:
                printf("Enter Register Number: ");
                scanf("%d", &req.reg_no);
                break;
            case 3:
                printf("Enter Register Number to Update: ");
                scanf("%d", &req.student_data.reg_no);
                printf("Enter New Details:\n");
                printf("Enter Name: ");
                scanf(" %[^\n]", req.student_data.name);
                printf("Enter Department: ");
                scanf(" %[^\n]", req.student_data.dept);
                printf("Enter Semester: ");
                scanf("%d", &req.student_data.semester);
                printf("Enter Marks for %d Subjects:\n", NUM_SUBJECTS);
                for (int i = 0; i < NUM_SUBJECTS; i++) {
                    printf("  Subject %d: ", i + 1);
                    scanf("%f", &req.student_data.marks[i]);
                }
                break;
            case 5:
            case 6:
                break;
            default:
                printf("Invalid Choice! Enter between 1 and 7.\n");
                continue;
        }

        write(sock, &req, sizeof(Request));

        memset(&res, 0, sizeof(Response));
        read(sock, &res, sizeof(Response));

        printf("\nServer Response: %s\n", res.message);

        if (res.success && res.count > 0) {
            for (int i = 0; i < res.count; i++) {
                print_student(res.students[i]);
            }
        }
    }

    close(sock);
    return 0;
}