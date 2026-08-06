#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BYTES 1000

// Function to encode an 8-bit character byte into a 12-bit Hamming string
void encode_hamming_byte(char byte, char *encoded_str) 
{
    int data[9]; // 1-based indexing for data bits (1..8)
    
    // Extract bits (MSB at pos 1, LSB at pos 8)
    for (int j = 7; j >= 0; j--) 
    {
        data[8 - j] = (byte >> j) & 1;
    }

    // 1-based indexing array for 12-bit Hamming code
    int ham[13] = {0};

    // Place data bits into non-power-of-2 positions
    ham[3]  = data[1];
    ham[5]  = data[2];
    ham[6]  = data[3];
    ham[7]  = data[4];
    ham[9]  = data[5];
    ham[10] = data[6];
    ham[11] = data[7];
    ham[12] = data[8];

    // Calculate Parity Bits (Even Parity)
    ham[1] = ham[3] ^ ham[5] ^ ham[7] ^ ham[9] ^ ham[11];
    ham[2] = ham[3] ^ ham[6] ^ ham[7] ^ ham[10] ^ ham[11];
    ham[4] = ham[5] ^ ham[6] ^ ham[7] ^ ham[12];
    ham[8] = ham[9] ^ ham[10] ^ ham[11] ^ ham[12];

    // Convert array to string output
    for (int i = 1; i <= 12; i++) 
    {
        encoded_str[i - 1] = ham[i] + '0';
    }
    encoded_str[12] = '\0';
}

int main() 
{
    FILE *fin = fopen("input.txt", "r");
    if (!fin) 
    {
        printf("[Sender Error] Could not open input.txt!, Make sure it exists.\n");
        return 1;
    }

    char message[MAX_BYTES];
    if (fgets(message, sizeof(message), fin) == NULL) 
    {
        printf("[Sender Error] input.txt is empty!\n");
        fclose(fin);
        return 1;
    }
    fclose(fin);

    int n = strlen(message);
    if (n > 0 && message[n - 1] == '\n') 
    {
        message[n - 1] = '\0';
        n--;
    }

    FILE *ftrans = fopen("transmission.txt", "w");
    if (!ftrans)
    {
        printf("[Sender Error] Could not create transmission.txt!\n");
        return 1;
    }

    // Process each character byte into Hamming Code and write continuously as one stream
    char encoded_frame[13];
    for (int i = 0; i < n; i++) 
    {
        encode_hamming_byte(message[i], encoded_frame);
        fprintf(ftrans, "%s", encoded_frame); // Print continuous string
    }

    fclose(ftrans);
    printf("[Sender] Message processed as a single continuous binary stream into transmission.txt!\n");
    return 0;
}