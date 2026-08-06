#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_MSG_LEN 1000
#define STREAM_SIZE 20000 
#define FRAME_SIZE 32
#define PACKET_BIT_SIZE 64

// PPP Special 8-bit String Constants
#define PPP_FLAG "01111110"  // 0x7E
#define PPP_ESC  "01111101"  // 0x7D

typedef struct 
{
    char url[50];
    char ip[20];
    char mac[20];
    int port;
} NetworkEntry;

// Conversion Utilities
void charToBinaryString(char ch, char outputStr[]);
void intTo16BitBinaryString(int num, char outputStr[]);
void ipToBinaryString(char ipStr[], char outputStr[]);
void macToBinaryString(char macStr[], char outputStr[]);
char binaryStringToChar(const char bitStr[]);

// Network Logic Setup
void initializeLookupTable(NetworkEntry table[]);
void printLookupTable(NetworkEntry table[]);
int resolveURL(NetworkEntry table[], char targetURL[], char destIP[], char destMAC[], int *destPort);
int readMessageFile(char fileMessage[]);

// Layers (Sender-side Encapsulation)
void ApplicationLayer(char fileMessage[], int msgLength, char appStream[]);
void TransportLayer(char appStream[], int srcPort, int destPort, char binSrcPort[], char binDestPort[], char transportStream[]);
void NetworkLayer(char transportStream[], char srcIP[], char destIP[], char binSrcIP[], char binDestIP[], char networkStream[]);
void DataLinkLabel(char networkStream[], char srcMAC[], char destMAC[], char binSrcMAC[], char binDestMAC[], char finalStream[]);

// PPP Processing Layers
void pppByteStuffing(char finalStream[], char stuffedStream[]);
void writeTransmissionFile(char stuffedStream[]);
void readTransmissionFile(char receivedStream[]);
void pppByteUnstuffing(char receivedStream[], char unstuffedStream[]);

// Receiver Decapsulation
void ReceiverProcess(char unstuffedStream[]);

int main()
{
    srand(time(NULL));
    char srcIP[20] = "192.168.1.5";
    char srcMAC[20] = "AA:BB:CC:DD:EE:11";
    int srcPort = 49152 + (rand() % (65535 - 49152 + 1));

    NetworkEntry table[3];
    initializeLookupTable(table);
    printLookupTable(table); // Re-added display block

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

    // ==========================================
    // 1. SENDER PHASE (Reads message.txt)
    // ==========================================
    char fileMessage[MAX_MSG_LEN];
    int msgLength = readMessageFile(fileMessage);
    if (msgLength <= 0) return 1; 

    char appStream[STREAM_SIZE] = "";
    char transportStream[STREAM_SIZE] = "";
    char networkStream[STREAM_SIZE] = "";
    char finalStream[STREAM_SIZE] = "";
    char stuffedStream[STREAM_SIZE] = "";

    char binSrcPort[17], binDestPort[17];
    intTo16BitBinaryString(srcPort, binSrcPort);
    intTo16BitBinaryString(destPort, binDestPort);

    char binSrcIP[33], binDestIP[33];
    ipToBinaryString(srcIP, binSrcIP);
    ipToBinaryString(destIP, binDestIP);

    char binSrcMAC[49], binDestMAC[49];
    macToBinaryString(srcMAC, binSrcMAC);
    macToBinaryString(destMAC, binDestMAC);

    // Run traditional header wrapping with detailed visualization
    ApplicationLayer(fileMessage, msgLength, appStream);
    TransportLayer(appStream, srcPort, destPort, binSrcPort, binDestPort, transportStream);
    NetworkLayer(transportStream, srcIP, destIP, binSrcIP, binDestIP, networkStream);
    DataLinkLabel(networkStream, srcMAC, destMAC, binSrcMAC, binDestMAC, finalStream);

    // Apply PPP Byte Stuffing logic and output to wire file
    pppByteStuffing(finalStream, stuffedStream);
    writeTransmissionFile(stuffedStream);

    printf("\n>>> SENDER COMPLETE: Data written cleanly to 'transmission.txt' <<<\n\n");

    // ==========================================
    // 2. RECEIVER PHASE (Reads transmission.txt)
    // ==========================================
    printf("=====================================================================\n");
    printf("                         RECEIVER ENGINE                             \n");
    printf("=====================================================================\n");
    
    char receivedStream[STREAM_SIZE] = "";
    char unstuffedStream[STREAM_SIZE] = "";

    readTransmissionFile(receivedStream);
    pppByteUnstuffing(receivedStream, unstuffedStream);
    ReceiverProcess(unstuffedStream);

    return 0;
}

// --- PPP SENDER LOGIC ---

void pppByteStuffing(char finalStream[], char stuffedStream[])
{
    printf("=====================================================================\n");
    printf("                    PPP FRAME BYTE STUFFING                          \n");
    printf("=====================================================================\n");

    int len = (int)strlen(finalStream);
    stuffedStream[0] = '\0';

    // 1. Add front framing headers per the diagram: Flag(1B) + Address(1B) + Control(1B) + Protocol(2B)
    strcat(stuffedStream, PPP_FLAG);       // Flag: 01111110
    strcat(stuffedStream, "11111111");     // Address: 11111111
    strcat(stuffedStream, "11000000");     // Control: 11000000
    strcat(stuffedStream, "0000000000100001"); // Protocol: 2-Bytes payload marker

    // 2. Look at data payload 8-bits (1 byte) at a time and escape if necessary
    char currentByte[9];
    for (int i = 0; i < len; i += 8)
    {
        strncpy(currentByte, &finalStream[i], 8);
        currentByte[8] = '\0';

        if (strcmp(currentByte, PPP_FLAG) == 0 || strcmp(currentByte, PPP_ESC) == 0)
        {
            // If native pattern hits flag or escape, stuff the escape code first!
            strcat(stuffedStream, PPP_ESC);
            strcat(stuffedStream, currentByte);
        }
        else
        {
            strcat(stuffedStream, currentByte);
        }
    }

    // 3. Add back framing footer: FCS Checksum Placeholder (2B) + Flag(1B)
    strcat(stuffedStream, "0000000000000000"); // FCS
    strcat(stuffedStream, PPP_FLAG);       // Tail Flag

    printf("Stuffed PPP Transmission Stream:\n%s\n", stuffedStream);
    printf("Total Stuffed Stream Size: %d bits\n\n", (int)strlen(stuffedStream));
}

void writeTransmissionFile(char stuffedStream[])
{
    FILE *fOut = fopen("transmission.txt", "w");
    if (fOut != NULL)
    {
        fprintf(fOut, "%s", stuffedStream);
        fclose(fOut);
    }
}

// --- PPP RECEIVER LOGIC ---

void readTransmissionFile(char receivedStream[])
{
    FILE *fIn = fopen("transmission.txt", "r");
    if (fIn == NULL)
    {
        printf("Error: Could not open the transmission wire data file.\n");
        return;
    }
    fscanf(fIn, "%s", receivedStream);
    fclose(fIn);
    printf("[Receiver] Stuffed Bit Stream read from wire file:\n%s\n\n", receivedStream);
}

void pppByteUnstuffing(char receivedStream[], char unstuffedStream[])
{
    printf("=====================================================================\n");
    printf("                   PPP FRAME BYTE UNSTUFFING                         \n");
    printf("=====================================================================\n");

    int totalLen = (int)strlen(receivedStream);
    unstuffedStream[0] = '\0';

    // Discard front headers (1B Flag + 1B Address + 1B Control + 2B Protocol = 5 Bytes = 40 bits)
    // Discard back footers (2B FCS + 1B Flag = 3 Bytes = 24 bits)
    int payloadStart = 40;
    int payloadEnd = totalLen - 24;

    char currentByte[9];
    for (int i = payloadStart; i < payloadEnd; i += 8)
    {
        strncpy(currentByte, &receivedStream[i], 8);
        currentByte[8] = '\0';

        if (strcmp(currentByte, PPP_ESC) == 0)
        {
            // Encountered escape sequence! Drop this byte, jump forward, and read true literal data
            i += 8;
            strncpy(currentByte, &receivedStream[i], 8);
            currentByte[8] = '\0';
            strcat(unstuffedStream, currentByte);
        }
        else
        {
            strcat(unstuffedStream, currentByte);
        }
    }
    printf("[Receiver] Clean Unstuffed Payload Bit Stream:\n%s\n\n", unstuffedStream);
    printf("Total Unstuffed Payload Size: %d bits\n\n", (int)strlen(unstuffedStream));
}

void ReceiverProcess(char unstuffedStream[])
{
    printf("=====================================================================\n");
    printf("                    RECEIVER PACKET PARSING                          \n");
    printf("=====================================================================\n");

    int bitOffset = 0;

    char recSrcMAC[49], recDestMAC[49];
    strncpy(recSrcMAC, &unstuffedStream[bitOffset], 48); recSrcMAC[48] = '\0'; bitOffset += 48;
    strncpy(recDestMAC, &unstuffedStream[bitOffset], 48); recDestMAC[48] = '\0'; bitOffset += 48;

    char recSrcIP[33], recDestIP[33];
    strncpy(recSrcIP, &unstuffedStream[bitOffset], 32); recSrcIP[32] = '\0'; bitOffset += 32;
    strncpy(recDestIP, &unstuffedStream[bitOffset], 32); recDestIP[32] = '\0'; bitOffset += 32;

    char recSrcPort[17], recDestPort[17];
    strncpy(recSrcPort, &unstuffedStream[bitOffset], 16); recSrcPort[16] = '\0'; bitOffset += 16;
    strncpy(recDestPort, &unstuffedStream[bitOffset], 16); recDestPort[16] = '\0'; bitOffset += 16;

    printf(">>> Decapsulating Headers extracted from Stream <<<\n");
    printf("Extracted Src MAC:   %s\n", recSrcMAC);
    printf("Extracted Dest MAC:  %s\n", recDestMAC);
    printf("Extracted Src IP:    %s\n", recSrcIP);
    printf("Extracted Dest IP:   %s\n", recDestIP);
    printf("Extracted Src Port:  %s\n", recSrcPort);
    printf("Extracted Dest Port: %s\n", recDestPort);
    printf("---------------------------------------------------------------------\n");

    // Reconstruct raw Text from remaining message bits
    char finalMessageText[MAX_MSG_LEN] = "";
    char singleCharBits[9];
    int txtIndex = 0;

    while (unstuffedStream[bitOffset] != '\0' && txtIndex < MAX_MSG_LEN - 1)
    {
        strncpy(singleCharBits, &unstuffedStream[bitOffset], 8);
        singleCharBits[8] = '\0';
        
        finalMessageText[txtIndex++] = binaryStringToChar(singleCharBits);
        bitOffset += 8;
    }
    finalMessageText[txtIndex] = '\0';

    printf("Parsed Message Output text string: \"%s\"\n", finalMessageText);

   
    // Save output down to a file for checking
    FILE *fOut = fopen("output.txt", "w");
    if (fOut != NULL)
    {
        fprintf(fOut, "%s", finalMessageText);
        fclose(fOut);
        printf("Success: Decoded message written completely to 'output.txt'!\n");
    }
}

// --- CORE UTILITY HELPER BLOCKS ---

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
    FILE *filePtr = fopen("input.txt", "r");
    if (filePtr == NULL) 
    {
        printf("\nError: Could not open 'message.txt'. Creating a demo file for you.\n");
        FILE *fWrite = fopen("message.txt", "w");
        fprintf(fWrite, "hi hello");
        fclose(fWrite);
        filePtr = fopen("message.txt", "r");
    }
    int msgLength = 0, ch;
    while ((ch = fgetc(filePtr)) != EOF && msgLength < MAX_MSG_LEN - 1) 
    {
        fileMessage[msgLength++] = (char)ch;
    }
    fileMessage[msgLength] = '\0';
    fclose(filePtr);
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

    strcpy(finalStream, binSrcMAC);
    strcat(finalStream, binDestMAC);
    strcat(finalStream, networkStream);

    printf("Final Data Link Bit Stream:\n%s\n", finalStream);
    printf("Total Final Stream Bits: %d bits\n\n", (int)strlen(finalStream));
}

void charToBinaryString(char ch, char outputStr[]) 
{
    unsigned char uCh = (unsigned char)ch;
    for (int i = 7; i >= 0; i--) outputStr[7 - i] = ((uCh >> i) & 1) ? '1' : '0';
    outputStr[8] = '\0';
}

char binaryStringToChar(const char bitStr[])
{
    char val = 0;
    for (int i = 0; i < 8; i++)
    {
        val <<= 1;
        if (bitStr[i] == '1') val |= 1;
    }
    return val;
}

void intTo16BitBinaryString(int num, char outputStr[]) 
{
    for (int i = 15; i >= 0; i--) outputStr[15 - i] = ((num >> i) & 1) ? '1' : '0';
    outputStr[16] = '\0';
}

void ipToBinaryString(char ipStr[], char outputStr[]) 
{
    int o1, o2, o3, o4;
    sscanf(ipStr, "%d.%d.%d.%d", &o1, &o2, &o3, &o4);
    char temp[9]; outputStr[0] = '\0';
    charToBinaryString((char)o1, temp); strcat(outputStr, temp);
    charToBinaryString((char)o2, temp); strcat(outputStr, temp);
    charToBinaryString((char)o3, temp); strcat(outputStr, temp);
    charToBinaryString((char)o4, temp); strcat(outputStr, temp);
}

void macToBinaryString(char macStr[], char outputStr[]) 
{
    int m1, m2, m3, m4, m5, m6;
    sscanf(macStr, "%x:%x:%x:%x:%x:%x",  &m1, &m2, &m3, &m4, &m5, &m6);
    char temp[9]; outputStr[0] = '\0';
    charToBinaryString((char)m1, temp); strcat(outputStr, temp);
    charToBinaryString((char)m2, temp); strcat(outputStr, temp);
    charToBinaryString((char)m3, temp); strcat(outputStr, temp);
    charToBinaryString((char)m4, temp); strcat(outputStr, temp);
    charToBinaryString((char)m5, temp); strcat(outputStr, temp);
    charToBinaryString((char)m6, temp); strcat(outputStr, temp);
}