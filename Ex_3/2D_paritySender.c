#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BYTES 1000

int main()
{
    FILE *fin = fopen("input.txt", "r");
    if (!fin)
    {
        printf("[Sender Error]:Could not open input.txt!, Make sure it exists.\n");
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

    int grid[MAX_BYTES][8];
    int row_parity[MAX_BYTES];
    int col_parity[8] = {0};
    int corner_parity = 0;

    // Convert characters to 8 bits & compute Row Parity
    for (int i = 0; i < n; i++) 
    {
        int r_sum = 0;
        for (int j = 7; j >= 0; j--) 
        {
            int bit = (message[i] >> j) & 1;
            grid[i][7 - j] = bit;
            r_sum ^= bit;
        }
        row_parity[i] = r_sum;
    }

    // Compute Column Parity & Corner Bit
    for (int j = 0; j < 8; j++) 
    {
        for (int i = 0; i < n; i++) 
        {
            col_parity[j] ^= grid[i][j];
        }
        corner_parity ^= col_parity[j];
    }

    // Save to transmission.txt
    FILE *ftrans = fopen("transmission.txt", "w");
    if (!ftrans) 
    {
        printf("[Sender Error] Could not create transmission.txt!\n");
        return 1;
    }

    // Data rows + Row Parity
    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < 8; j++) 
        {
            fprintf(ftrans, "%d", grid[i][j]);
        }
        fprintf(ftrans, "%d\n", row_parity[i]);
    }

    // Column Parity + Corner Parity (Last line)
    for (int j = 0; j < 8; j++)
    {
        fprintf(ftrans, "%d", col_parity[j]);
    }
    fprintf(ftrans, "%d\n", corner_parity);

    fclose(ftrans);
    printf("[Sender] Message processed and transmission.txt successfully created!\n");
    return 0;
}