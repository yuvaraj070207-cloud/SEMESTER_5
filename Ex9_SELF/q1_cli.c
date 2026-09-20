// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char input_string[BUFFER_SIZE];

    // 1. Get input string from user
    printf("Enter a string to check for palindrome: ");
    if (fgets(input_string, sizeof(input_string), stdin) == NULL) {
        return -1;
    }

    // Strip newline from fgets
    input_string[strcspn(input_string, "\n")] = 0;

    // 2. Create TCP Socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 3. Connect to Server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    // 4. Send string to Server
    write(sock, input_string, strlen(input_string));

    // 5. Read response from Server
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    printf("Server Response: %s\n", buffer);

    // 6. Close socket
    close(sock);
    return 0;
}