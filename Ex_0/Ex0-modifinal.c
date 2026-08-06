#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_MSG_LEN 1000
#define STREAM_SIZE 10000 
#define FRAME_SIZE 32
#define PACKET_BIT_SIZE 64

// Lookup table structure
typedef struct 
{
    char url[50];
    char ip[20];
    char mac[20];
    int port;
} NetworkEntry;

//conversions
void charToBinaryString(char ch, char outputStr[]);
void intTo16BitBinaryString(int num, char outputStr[]);
void ipToBinaryString(char ipStr[], char outputStr[]);
void macToBinaryString(char macStr[], char outputStr[]);

void initializeLookupTable(NetworkEntry table[]);
void printLookupTable(NetworkEntry table[]);
int resolveURL(NetworkEntry table[], char targetURL[], char destIP[], char destMAC[], int *destPort);
int readMessageFile(char fileMessage[]);

void ApplicationLayer(char fileMessage[], int msgLength, char appStream[]);
void TransportLayer(char appStream[], int srcPort, int destPort, char binSrcPort[], char binDestPort[], char transportStream[]);
void NetworkLayer(char transportStream[], char srcIP[], char destIP[], char binSrcIP[], char binDestIP[], char networkStream[]);
void DataLinkLabel(char networkStream[], char srcMAC[], char destMAC[], char binSrcMAC[], char binDestMAC[], char finalStream[]);

void segmentAndFrameStream(char finalStream[], char srcMAC[], char destMAC[], char srcIP[], char destIP[]);

int main()
{
    srand(time(NULL));
    char srcIP[20] = "192.168.1.5";
    char srcMAC[20] = "AA:BB:CC:DD:EE:11";
    int srcPort = 49152 + (rand() % (65535 - 49152 + 1));

    NetworkEntry table[3];
    initializeLookupTable(table);
    printLookupTable(table);

    char inputURL[50];
    char destIP[20], destMAC[20];
    int destPort;

    printf("Enter the URL: ");
    scanf("%49s", inputURL);

    if (resolveURL(table, inputURL, destIP, destMAC, &destPort) == -1) 
    {
        printf("\nError: URL not found in the local table.\n");
        return 1;
    }

    char fileMessage[MAX_MSG_LEN];
    int msgLength = readMessageFile(fileMessage);
    if (msgLength <= 0) 
    {
        return 1; 
    }

    //dynamic processing streams
    char appStream[STREAM_SIZE] = "";
    char transportStream[STREAM_SIZE] = "";
    char networkStream[STREAM_SIZE] = "";
    char finalStream[STREAM_SIZE] = "";

    // Pre-compute binary transformations for headers
    char binSrcPort[17], binDestPort[17];
    intTo16BitBinaryString(srcPort, binSrcPort);
    intTo16BitBinaryString(destPort, binDestPort);

    char binSrcIP[33], binDestIP[33];
    ipToBinaryString(srcIP, binSrcIP);
    ipToBinaryString(destIP, binDestIP);

    char binSrcMAC[49], binDestMAC[49];
    macToBinaryString(srcMAC, binSrcMAC);
    macToBinaryString(destMAC, binDestMAC);

    
    ApplicationLayer(fileMessage, msgLength, appStream);
    TransportLayer(appStream, srcPort, destPort, binSrcPort, binDestPort, transportStream);
    NetworkLayer(transportStream, srcIP, destIP, binSrcIP, binDestIP, networkStream);
    DataLinkLabel(networkStream, srcMAC, destMAC, binSrcMAC, binDestMAC, finalStream);

    //FRAMING
    segmentAndFrameStream(finalStream, srcMAC, destMAC, srcIP, destIP);

    return 0;
}

void initializeLookupTable(NetworkEntry table[])
{
    strcpy(table[0].url, "www.gmail.com");
    strcpy(table[0].ip, "142.250.190.46");
    strcpy(table[0].mac, "00:11:22:33:44:55");
    table[0].port = 443;

    strcpy(table[1].url, "www.facebook.com");
    strcpy(table[1].ip, "157.240.22.35");
    strcpy(table[1].mac, "66:77:88:99:AA:BB");
    table[1].port = 80;

    strcpy(table[2].url, "www.yahoo.com");
    strcpy(table[2].ip, "98.137.11.163");
    strcpy(table[2].mac, "CC:DD:EE:FF:00:11");
    table[2].port = 8080;
}

void printLookupTable(NetworkEntry table[])
{
    printf("=====================================================================\n");
    printf("                      NETWORK LOOKUP TABLE                           \n");
    printf("=====================================================================\n");
    printf("%-20s %-16s %-20s %-6s\n", "URL", "IP Address", "MAC Address", "Port");
    printf("---------------------------------------------------------------------\n");
    for (int i = 0; i < 3; i++)
    {
        printf("%-20s %-16s %-20s %-6d\n", table[i].url, table[i].ip, table[i].mac, table[i].port);
    }
    printf("=====================================================================\n\n");
}

int resolveURL(NetworkEntry table[], char targetURL[], char destIP[], char destMAC[], int *destPort)
{
    for (int i = 0; i < 3; i++) 
    {
        if (strcmp(table[i].url, targetURL) == 0)
        {
            strcpy(destIP, table[i].ip);
            strcpy(destMAC, table[i].mac);
            *destPort = table[i].port;
            return i;
        }
    }
    return -1;
}

int readMessageFile(char fileMessage[])
{
    FILE *fp = fopen("message.txt", "r");
    if (fp == NULL) 
    {
        printf("\nError: Could not open 'message.txt' in this folder.\n");
        return -1;
    }
    int msgLength = 0, ch;
    while ((ch = fgetc(fp)) != EOF && msgLength < MAX_MSG_LEN - 1) 
    {
        fileMessage[msgLength++] = (char)ch;
    }
    fileMessage[msgLength] = '\0';
    fclose(fp);

    if (msgLength == 0) 
    {
        printf("\nError: 'message.txt' is empty!\n");
        return 0;
    }
    return msgLength;
}

void ApplicationLayer(char fileMessage[], int msgLength, char appStream[])
{
    printf("=====================================================================\n");
    printf("                        APPLICATION LAYER                            \n");
    printf("=====================================================================\n");
    printf("Raw Message: \"%s\"\n", fileMessage);
    printf("Total Payload Size: %d bytes\n", msgLength);
    printf("---------------------------------------------------------------------\n");

    appStream[0] = '\0';
    char tempBin[9];
    for (int i = 0; i < msgLength; i++)
    {
        charToBinaryString(fileMessage[i], tempBin);
        strcat(appStream, tempBin);
    }
    printf("Application Bit Stream:\n%s\n", appStream);
    printf("Total Stream Bits: %d bits\n\n", (int)strlen(appStream));
}

void TransportLayer(char appStream[], int srcPort, int destPort, char binSrcPort[], char binDestPort[], char transportStream[])
{
    printf("=====================================================================\n");
    printf("                        TRANSPORT LAYER                              \n");
    printf("=====================================================================\n");
    printf("Source Port     : %-5d -> Binary: %s\n", srcPort, binSrcPort);
    printf("Destination Port: %-5d -> Binary: %s\n", destPort, binDestPort);
    printf("---------------------------------------------------------------------\n");

    // Prepend headers to the previous layer's stream
    strcpy(transportStream, binSrcPort);
    strcat(transportStream, binDestPort);
    strcat(transportStream, appStream);

    printf("Transport Layer Stream (Ports + App Data):\n%s\n", transportStream);
    printf("Total Stream Bits: %d bits\n\n", (int)strlen(transportStream));
}

void NetworkLayer(char transportStream[], char srcIP[], char destIP[], char binSrcIP[], char binDestIP[], char networkStream[])
{
    printf("=====================================================================\n");
    printf("                         NETWORK LAYER                               \n");
    printf("=====================================================================\n");
    printf("Source IP     : %-15s -> Binary: %s\n", srcIP, binSrcIP);
    printf("Destination IP: %-15s -> Binary: %s\n", destIP, binDestIP);
    printf("---------------------------------------------------------------------\n");

    // Prepend IP headers to the transport stream
    strcpy(networkStream, binSrcIP);
    strcat(networkStream, binDestIP);
    strcat(networkStream, transportStream);

    printf("Network Layer Stream (IPs + Transport Data):\n%s\n", networkStream);
    printf("Total Stream Bits: %d bits\n\n", (int)strlen(networkStream));
}

void DataLinkLabel(char networkStream[], char srcMAC[], char destMAC[], char binSrcMAC[], char binDestMAC[], char finalStream[])
{
    printf("=====================================================================\n");
    printf("                        DATA LINK LAYER                              \n");
    printf("=====================================================================\n");
    printf("Source MAC      : %-17s -> Binary: %s\n", srcMAC, binSrcMAC);
    printf("Destination MAC : %-17s -> Binary: %s\n", destMAC, binDestMAC);
    printf("---------------------------------------------------------------------\n");

    // Prepend MAC headers to the network stream
    strcpy(finalStream, binSrcMAC);
    strcat(finalStream, binDestMAC);
    strcat(finalStream, networkStream);

    printf("Final Data Link Bit Stream:\n%s\n", finalStream);
    printf("Total Final Stream Bits: %d bits\n\n", (int)strlen(finalStream));
}

void segmentAndFrameStream(char finalStream[], char srcMAC[], char destMAC[], char srcIP[], char destIP[])
{
    printf("=====================================================================\n");
    printf("                STREAM FRAGMENTATION & FRAMING                       \n");
    printf("=====================================================================\n");
    
    int originalLen = (int)strlen(finalStream);
    printf("Initial Bit Count: %d bits\n", originalLen);
    printf("Target Frame Size: %d bits | Target Packet Cluster Unit: %d bits\n", FRAME_SIZE, PACKET_BIT_SIZE);
    printf("---------------------------------------------------------------------\n");

    // Calculate padding requirements for the stream based on Frame boundaries
    int totalFrames = originalLen / FRAME_SIZE;
    if (originalLen % FRAME_SIZE != 0)
    {
        totalFrames++;
    }
    int paddedLen = totalFrames * FRAME_SIZE;
    
     // calculating total number of packets
    int totalPackets = paddedLen / PACKET_BIT_SIZE;
    if (paddedLen % PACKET_BIT_SIZE != 0)
    {
        totalPackets++;
    }
    
    // Create a padded working copy of the final bit stream
    char paddedStream[STREAM_SIZE];
    strcpy(paddedStream, finalStream);
    int paddedBitCount=0;
    
    while ((int)strlen(paddedStream) < paddedLen)
    {
        strcat(paddedStream, "0"); // Pad right side with zeros
        paddedBitCount++;
    }

    printf("Padded Stream Bit Count: %d bits\n", paddedLen);
    printf("Total no of padded bits: %d bits\n",paddedBitCount);
    printf("Padded Final Bit Stream:\n%s\n", paddedStream);
   
    printf("---------------------------------------------------------------------\n\n");

    int frameNo=0;
    int packetNo=0;
    
    //cutting the final padded data bits into frames of 32 bit data
    for (int f = 0; f < totalFrames; f++)
    {
        char frameBuffer[FRAME_SIZE + 1];
        strncpy(frameBuffer, &paddedStream[f * FRAME_SIZE], FRAME_SIZE);
        frameBuffer[FRAME_SIZE] = '\0';

        // Tracking each frame 
        int currentBitOffset = f * FRAME_SIZE;
        int packetNumber = (currentBitOffset / PACKET_BIT_SIZE) + 1;

        printf("Frame Bits     : %s\n", frameBuffer);
        printf("Frame No.   : %d\n", f + 1);
        printf("Packet No.  : %d\n", packetNumber);
        printf("Source MAC  : %s\n", srcMAC);
        printf("Dest MAC    : %s\n", destMAC);
        printf("Source IP   : %s\n", srcIP);
        printf("Dest IP     : %s\n", destIP);
        printf("---------------------------------------------------------------------\n");

    }
     printf("Total Number of frames: %d\n", totalFrames);
      printf("Total Number of packets: %d\n", totalPackets);
    printf("Transmission Completed. All frames sent cleanly.\n");
}

// Conversion implementation blocks
void charToBinaryString(char ch, char outputStr[]) 
{
    unsigned char uCh = (unsigned char)ch;  //preventing negative sign errors
    //each character is 8 bit and integer is 16 bit 
    for (int i = 7; i >= 0; i--)
    {
        outputStr[7 - i] = ((uCh >> i) & 1) ? '1' : '0';
    }
    outputStr[8] = '\0';
}

void intTo16BitBinaryString(int num, char outputStr[]) 
{
    for (int i = 15; i >= 0; i--)
    {
        outputStr[15 - i] = ((num >> i) & 1) ? '1' : '0';   //masking the LSB using bitwise AND to extract the last bit
    }
    outputStr[16] = '\0';
}

void ipToBinaryString(char ipStr[], char outputStr[]) 
{
    int o1, o2, o3, o4;
    sscanf(ipStr, "%d.%d.%d.%d", &o1, &o2, &o3, &o4);
    char temp[9];
    outputStr[0] = '\0';
    
    charToBinaryString((char)o1, temp); strcat(outputStr, temp);
    charToBinaryString((char)o2, temp); strcat(outputStr, temp);
    charToBinaryString((char)o3, temp); strcat(outputStr, temp);
    charToBinaryString((char)o4, temp); strcat(outputStr, temp);
}

void macToBinaryString(char macStr[], char outputStr[]) 
{
    int m1, m2, m3, m4, m5, m6;
    sscanf(macStr, "%x:%x:%x:%x:%x:%x", &m1, &m2, &m3, &m4, &m5, &m6);
    char temp[9];
    outputStr[0] = '\0';
    
    charToBinaryString((char)m1, temp); strcat(outputStr, temp);
    charToBinaryString((char)m2, temp); strcat(outputStr, temp);
    charToBinaryString((char)m3, temp); strcat(outputStr, temp);
    charToBinaryString((char)m4, temp); strcat(outputStr, temp);
    charToBinaryString((char)m5, temp); strcat(outputStr, temp);
    charToBinaryString((char)m6, temp); strcat(outputStr, temp);
}