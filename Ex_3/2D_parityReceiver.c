#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BYTES 1000

// Function to save the modified grid back to transmission.txt
void save_to_transmission_file(int grid[MAX_BYTES][8], int row_parity[MAX_BYTES],int col_parity[8], int corner_parity, int n) 
{
    FILE *ftrans = fopen("transmission.txt", "w");
    if (!ftrans) 
    {
        printf("[Receiver Error] Could not update transmission.txt!\n");
        return;
    }

    // To write Data rows and row parity
    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < 8; j++) 
        {
            fprintf(ftrans, "%d", grid[i][j]);
        }
        fprintf(ftrans, "%d\n", row_parity[i]);
    }

    // To write column parities and corner parity (Last line)
    for (int j = 0; j < 8; j++) 
    {
        fprintf(ftrans, "%d", col_parity[j]);
    }
    fprintf(ftrans, "%d\n", corner_parity);

    fclose(ftrans);
    printf("[System] transmission.txt file updated on disk!\n");
}

// Function to perform parity checking & pinpoint single-bit error locations
void perform_parity_check(int grid[MAX_BYTES][8], int row_parity[MAX_BYTES],int col_parity[8], int corner_parity, int n) {
    int error_row = -1;
    int error_col = -1;

    //Check Row Parities
    for (int i = 0; i < n; i++) 
    {
        int r_sum = 0;
        for (int j = 0; j < 8; j++) 
        {
            r_sum ^= grid[i][j];
        }
        if (r_sum != row_parity[i]) 
        {
            error_row = i;
        }
    }

    //Check Column Parities
    for (int j = 0; j < 8; j++) 
    {
        int c_sum = 0;
        for (int i = 0; i < n; i++) 
        {
            c_sum ^= grid[i][j];
        }
        if (c_sum != col_parity[j]) 
        {
            error_col = j;
        }
    }

    printf("\n--------------------------------------------------\n");
    if (error_row == -1 && error_col == -1) 
    {
        printf("[Receiver Result] NO ERROR DETECTED by the receiver!\n");
        
        // reconstruct string
        char reconstructed[MAX_BYTES] = {0};
        for (int i = 0; i < n; i++) 
        {
            char ch = 0;
            for (int j = 0; j < 8; j++)
            {
                ch = (ch << 1) | grid[i][j];
            }
            reconstructed[i] = ch;
        }
        printf("[Receiver Decoded Message] %s\n", reconstructed);
    } 
    else if (error_row != -1 && error_col != -1) 
    {
        printf("[Receiver Result] ERROR DETECTED at bit location (%d, %d)!\n", 
               error_row + 1, error_col + 1); // 1-based display
    } 
    else 
    {
        printf("[Receiver Result] ERROR DETECTED in parity bits!\n");
    }
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

    char matrix_data[MAX_BYTES + 1][20];
    int total_lines = 0;

    // Read lines from transmission.txt
    while (fgets(matrix_data[total_lines], sizeof(matrix_data[0]), ftrans) != NULL) 
    {
        int len = strlen(matrix_data[total_lines]);
        if (len > 0 && matrix_data[total_lines][len - 1] == '\n')
        {
            matrix_data[total_lines][len - 1] = '\0';
        }
        total_lines++;
    }
    fclose(ftrans);

    if (total_lines < 2)
    {
        printf("[Receiver Error] Invalid or corrupted transmission file!\n");
        return 1;
    }

    int n = total_lines - 1; // Total character rows, last row is the column parity row
    int grid[MAX_BYTES][8];
    int row_parity[MAX_BYTES];
    int col_parity[8];
    int corner_parity;

    //parsing Data & Row Parities
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            grid[i][j] = matrix_data[i][j] - '0';   //character to integer conversion ASCII of '0'=48,'1'=49
        }
        row_parity[i] = matrix_data[i][8] - '0';
    }

    // parsing Column Parities & Corner Parity
    for (int j = 0; j < 8; j++) 
    {
        col_parity[j] = matrix_data[n][j] - '0';
    }
    corner_parity = matrix_data[n][8] - '0';

    // Continuous Loop for Modification & Checking
    char choice;
    while (1) 
    {
        // Run Parity Check on current state
        perform_parity_check(grid, row_parity, col_parity, corner_parity, n);

        printf("\nDo you want to modify a data bit? (y/n): ");
        scanf(" %c", &choice);

        if (choice == 'n' || choice == 'N') 
        {
            printf("\nExiting receiver program cleanly.\n");
            break;
        } 
        else if (choice == 'y' || choice == 'Y') 
        {
            int target_row, target_col;
            printf("Enter Row number (1 to %d): ", n);
            scanf("%d", &target_row);
            printf("Enter Column number (1 to 8): ");
            scanf("%d", &target_col);

            int r = target_row - 1;
            int c = target_col - 1;

            if (r >= 0 && r < n && c >= 0 && c < 8) 
            {
                // Complement (flip) bit
                grid[r][c] ^= 1;
                printf("\n[Simulator] Flipped bit at (%d, %d) to %d.\n", target_row, target_col, grid[r][c]);

                // Update physical file on disk
                save_to_transmission_file(grid, row_parity, col_parity, corner_parity, n);
            } 
            else 
            {
                printf("[Error] Invalid Row or Column selection!\n");
            }
        } 
        else 
        {
            printf("[Error] Invalid option! Please enter 'y' or 'n'.\n");
        }
    }

    return 0;
}