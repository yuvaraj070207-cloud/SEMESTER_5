#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_BYTES 1000

// Function to save the full continuous stream back to transmission.txt
void save_to_transmission_file(char *stream) 
{
    FILE *ftrans = fopen("transmission.txt", "w");
    if (!ftrans) 
    {
        printf("[Receiver Error] Could not update transmission.txt!\n");
        return;
    }

    fprintf(ftrans, "%s", stream);

    fclose(ftrans);
    printf("[System] transmission.txt file updated on disk!\n");
}

// Function to check syndrome and correct single-bit errors across 12-bit chunks
void perform_hamming_check(char *stream, int total_bits) 
{
    int num_blocks = total_bits / 12;
    int any_error = 0;

    printf("\n--------------------------------------------------\n");
    for (int b = 0; b < num_blocks; b++) 
    {
        int ham[13];
        
        // Read 12-bit block into 1-indexed array
        for (int j = 1; j <= 12; j++) 
        {
            ham[j] = stream[b * 12 + (j - 1)] - '0';
        }

        // Calculate syndrome bits
        int s1 = ham[1] ^ ham[3] ^ ham[5] ^ ham[7] ^ ham[9] ^ ham[11];
        int s2 = ham[2] ^ ham[3] ^ ham[6] ^ ham[7] ^ ham[10] ^ ham[11];
        int s4 = ham[4] ^ ham[5] ^ ham[6] ^ ham[7] ^ ham[12];
        int s8 = ham[8] ^ ham[9] ^ ham[10] ^ ham[11] ^ ham[12];

        int syndrome = (s8 * 8) + (s4 * 4) + (s2 * 2) + (s1 * 1);  //it should come zero otherwise there is a syndrome(defect) in a paritcular bit

        if (syndrome != 0) 
        {
            any_error = 1;
            int global_err_bit = (b * 12) + syndrome;
            printf("[Receiver Result] ERROR DETECTED at Block %d! Bit Position inside block: %d (Global Bit Position: %d)\n", 
                   b + 1, syndrome, global_err_bit);
            
            // 1's complement flip to correct error in stream
            ham[syndrome] ^= 1;
            stream[(b * 12) + (syndrome - 1)] = ham[syndrome] + '0';
            printf("[Receiver Result] Error automatically corrected at Global Bit Position %d!\n", global_err_bit);
        }
    }

    if (!any_error) 
    {
        printf("[Receiver Result] NO ERROR DETECTED by the receiver!\n");
    }

    // Reconstruct string from clean data bits (positions 3, 5, 6, 7, 9, 10, 11, 12 per block)
    char reconstructed[MAX_BYTES] = {0};
    for (int b = 0; b < num_blocks; b++) 
    {
        int h[13];
        for (int j = 1; j <= 12; j++) 
        {
            h[j] = stream[b * 12 + (j - 1)] - '0';
        }

        char ch = 0;
        int data_positions[8] = {3, 5, 6, 7, 9, 10, 11, 12};
        for (int d = 0; d < 8; d++) 
        {
            ch = (ch << 1) | h[data_positions[d]];
        }
        reconstructed[b] = ch;
    }

    printf("[Receiver Decoded Message] %s\n", reconstructed);
    printf("--------------------------------------------------\n");
}

int main() 
{
    FILE *ftrans = fopen("transmission.txt", "r");
    if (!ftrans) 
    {
        printf("[Receiver Error] Could not open transmission.txt!\n");
        return 1;
    }

    char stream[MAX_BYTES * 12];
    if (fgets(stream, sizeof(stream), ftrans) == NULL) 
    {
        printf("[Receiver Error] transmission.txt is empty!\n");
        fclose(ftrans);
        return 1;
    }
    fclose(ftrans);

    int total_bits = strlen(stream);
    if (total_bits > 0 && stream[total_bits - 1] == '\n') 
    {
        stream[total_bits - 1] = '\0';
        total_bits--;
    }

    if (total_bits % 12 != 0 || total_bits == 0) 
    {
        printf("[Receiver Error] Invalid transmission bitstream length!\n");
        return 1;
    }

    char choice;
    while (1) 
    {
        // Run Syndrome Check on continuous stream
        perform_hamming_check(stream, total_bits);

        printf("\nDo you want to modify a bit in the transmission stream? (y/n): ");
        scanf(" %c", &choice);

        if (choice == 'n' || choice == 'N') {
            printf("\nExiting receiver program cleanly.\n");
            break;
        } 
        else if (choice == 'y' || choice == 'Y') 
        {
            int target_bit;
            printf("Enter 1-indexed Bit Position in the entire stream (1 to %d): ", total_bits);
            scanf("%d", &target_bit);

            int bit_idx = target_bit - 1;

            if (bit_idx >= 0 && bit_idx < total_bits) {
                // 1's complement bit flip directly in the stream
                stream[bit_idx] = (stream[bit_idx] == '1') ? '0' : '1';
                printf("\n[Simulator] Flipped bit at Global Bit Position %d to %c.\n", 
                       target_bit, stream[bit_idx]);

                // Update physical transmission file on disk
                save_to_transmission_file(stream);
            } else {
                printf("[Error] Invalid Bit selection!\n");
            }
        } 
        else {
            printf("[Error] Invalid option! Please enter 'y' or 'n'.\n");
        }
    }

    return 0;
}