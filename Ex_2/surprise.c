#include <stdio.h>
#include <stdlib.h>

int main()
{
    char c;
    int pos = 0;
    int n = 0;
    int check = 0;

    printf("Enter the number of bits for the bit string: ");
    scanf("%d", &n);
    int bits[n];
    
    for(int i = 0; i < n; i++)
    {
        printf("Enter the element of the string: bits[%d]: ", i);
        scanf("%d", &bits[i]);
    }

    // 1. Calculate original parity
    int ev_parity = 0;
    for(int i = 0; i < n; i++)
    {
        ev_parity ^= bits[i];
    }
    printf("The even parity bit for the given string is: %d\n", ev_parity);

    // 2. Fix the scan formatting (space before %c skips trailing newline)
    printf("Do u want to modify a bit?: (Y/N) ");
    scanf(" %c", &c); 
    
    if(c == 'Y' || c == 'y')
    {
        printf("Enter the 1-indexed position of the bit u want to modify: ");
        scanf("%d", &pos); // Changed from %s to %d

        // Flip bit using integer values 1 and 0 instead of char literals '1' and '0'
        if(pos >= 1 && pos <= n) {
            bits[pos - 1] = (bits[pos - 1] == 0) ? 1 : 0; 
        }

        // Recalculate parity of modified data
        for(int i = 0; i < n; i++)
        {
            check ^= bits[i];
        }
        
        // System check: Include the sent parity bit into the overall evaluation
        // If (check ^ ev_parity) != 0, a single error happened!
        if((check ^ ev_parity) != 0)
        {
            printf("Error detected!\n");
        }
        else
        {
            printf("No error detected (or an even number of errors occurred)!\n");
        }
    }
    else
    {
        printf("Program terminated successfully!\n");
    }
    return 0;
}
