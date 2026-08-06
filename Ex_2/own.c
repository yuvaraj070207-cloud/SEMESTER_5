#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main()
{
    char c;
    int pos=0;
    int n=0;
    int check=0;

    printf("Enter the number of bits for the bit string:");
    scanf("%d",&n);
    int bits[n];
    for(int i=0;i<n;i++)
    {
        printf("Enter the element of the string:bits[%d]:",i);
        scanf("%d",&bits[i]);
    }

    int ev_parity=0;
    for(int i=0;i<n;i++)
    {
        ev_parity ^= bits[i];
    }
    printf("The even parity bit for the given string is:%d\n",ev_parity);

    printf("Do u want to modify a bit?: (Y/N)");
    scanf("%s",&c);
    if(c=='Y' || c=='y')
    {
        printf("enter the position of the bit u want to modify:");
        scanf("%s",&pos);

        for(int i=0;i<n;i++)
        {
            if((i+1)==pos)
            {
                bits[i]=(bits[i]==0)? 1:0;     //flipping the bits 
            }
        }
        for(int i=0;i<n;i++)
        {
            check ^= bits[i];
        }
        if(check!=ev_parity)
        {
            printf("Error detected !");
        }
        else
        {
            printf("No error detected !");
        }

    }
    else
    {
        printf("program terminated successfully !");

    }

}