// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Helper function to check if a string is a palindrome
int is_palindrome(const char *str) {
    int start = 0;
    int end = strlen(str) - 1;

    while (start < end) {
        // Convert characters to lowercase for case-insensitive comparison
        if (tolower((unsigned char)str[start]) != tolower((unsigned char)str[end])) {
            return 0; // Not a palindrome
        }
        start++;
        end--;
    }
    return 1; // Is a palindrome
}

// Handler to prevent zombie processes from fork()
void sigchld_handler(int s) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // 1. Create TCP Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Clean up zombie processes automatically
    signal(SIGCHLD, sigchld_handler);

    // 2. Bind socket to PORT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 3. Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Palindrome TCP Server listening on port %d...\n", PORT);

    while (1) {
        // 4. Accept incoming client connection
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) continue;

        // 5. Fork a child process to handle connection concurrently
        if (fork() == 0) {
            close(server_fd); // Child process doesn't need listening socket

            char buffer[BUFFER_SIZE] = {0};
            int bytes_read = read(new_socket, buffer, BUFFER_SIZE - 1);

            if (bytes_read > 0) {
                // Strip trailing newline if present
                buffer[strcspn(buffer, "\r\n")] = 0;

                printf("[Child PID %d] Received string: \"%s\"\n", getpid(), buffer);

                // Perform palindrome check
                if (is_palindrome(buffer)) {
                    char *res = "Result: The string is a PALINDROME.";
                    write(new_socket, res, strlen(res));
                } else {
                    char *res = "Result: The string is NOT a palindrome.";
                    write(new_socket, res, strlen(res));
                }
            }

            close(new_socket);
            exit(0); // Child terminates after serving
        }

        close(new_socket); // Parent process closes copy of socket
    }

    close(server_fd);
    return 0;
}