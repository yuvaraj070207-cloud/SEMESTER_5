#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_MSG_LEN 1000
#define STREAM_SIZE 30000 

// PPP Framing Flag for Bit-Oriented Protocols
#define BIT_FLAG "01111110"

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
int readInputFile(char fileMessage[]);

// Layers (Sender-side Encapsulation)
void ApplicationLayer(char fileMessage[], int msgLength, char appStream[]);
void TransportLayer(char appStream[], int srcPort, int destPort, char binSrcPort[], char binDestPort[], char transportStream[]);
void NetworkLayer(char transportStream[], char srcIP[], char destIP[], char binSrcIP[], char binDestIP[], char networkStream[]);
void DataLinkLabel(char networkStream[], char srcMAC[], char destMAC[], char binSrcMAC[], char binDestMAC[], char finalStream[]);

// Bit Stuffing Engine (Framing Format: FLAG | STUFFED(HEADERS + DATA) | FLAG)
void BitStuffing(char finalStream[], char stuffedStream[]);
void writeTransmissionFile(char stuffedStream[]);
void readTransmissionFile(char receivedStream[]);
void BitUnstuffing(char receivedStream[], char unstuffedStream[]);

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

    // ==========================================
    // 1. SENDER PHASE (Reads inp.txt)
    // ==========================================
    char fileMessage[MAX_MSG_LEN];
    int msgLength = readInputFile(fileMessage);
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

    // Layer encapsulation flow
    ApplicationLayer(fileMessage, msgLength, appStream);
    TransportLayer(appStream, srcPort, destPort, binSrcPort, binDestPort, transportStream);
    NetworkLayer(transportStream, srcIP, destIP, binSrcIP, binDestIP, networkStream);
    DataLinkLabel(networkStream, srcMAC, destMAC, binSrcMAC, binDestMAC, finalStream);

    // Run custom Bit Stuffing logic & write out to tran.txt
    BitStuffing(finalStream, stuffedStream);
    writeTransmissionFile(stuffedStream);

    printf("\n>>> SENDER COMPLETE: Data written cleanly to 'tran.txt' <<<\n\n");

    // ==========================================
    // 2. RECEIVER PHASE (Reads tran.txt)
    // ==========================================
    printf("=====================================================================\n");
    printf("                         RECEIVER ENGINE                             \n");
    printf("=====================================================================\n");
    
    char receivedStream[STREAM_SIZE] = "";
    char unstuffedStream[STREAM_SIZE] = "";

    readTransmissionFile(receivedStream);
    BitUnstuffing(receivedStream, unstuffedStream);
    ReceiverProcess(unstuffedStream);

    return 0;
}

// --- BIT STUFFING IMPLEMENTATION ---

void BitStuffing(char finalStream[], char stuffedStream[])
{
    printf("=====================================================================\n");
    printf("                    PPP FRAME BIT STUFFING                           \n");
    printf("=====================================================================\n");
    printf("Framing Design: [FLAG] | [STUFFED(HEADERS + DATA payload)] | [FLAG]\n");
    printf("---------------------------------------------------------------------\n");

    int len = (int)strlen(finalStream);
    int outIdx = 0;

    // 1. Prepend the opening frame flag
    strcpy(stuffedStream, BIT_FLAG);
    outIdx = (int)strlen(stuffedStream);

    int consecutiveOnes = 0;

    // 2. Scan and stuff the composite block (Headers + Payload Data combined)
    for (int i = 0; i < len; i++)
    {
        char currentBit = finalStream[i];
        stuffedStream[outIdx++] = currentBit;

        if (currentBit == '1')
        {
            consecutiveOnes++;
            // Hit 5 consecutive ones? Inject a stuffed '0' bit right away
            if (consecutiveOnes == 5)
            {
                stuffedStream[outIdx++] = '0';
                consecutiveOnes = 0; 
            }
        }
        else
        {
            consecutiveOnes = 0;
        }
    }

    // 3. Append the trailing closing frame flag
    stuffedStream[outIdx] = '\0';
    strcat(stuffedStream, BIT_FLAG);

    printf("Stuffed Complete Frame Stream:\n%s\n", stuffedStream);
    printf("Total Bits Sent to Wire: %d bits\n\n", (int)strlen(stuffedStream));
}

void writeTransmissionFile(char stuffedStream[])
{
    FILE *fOut = fopen("tran.txt", "w");
    if (fOut != NULL)
    {
        fprintf(fOut, "%s", stuffedStream);
        fclose(fOut);
    }
}

// --- RECEIVER LOGIC ---

void readTransmissionFile(char receivedStream[])
{
    FILE *fIn = fopen("tran.txt", "r");
    if (fIn == NULL)
    {
        printf("Error: Could not open the 'tran.txt' wire data file.\n");
        return;
    }
    fscanf(fIn, "%s", receivedStream);
    fclose(fIn);
    printf("[Wire Status] Ingested Frame read from 'tran.txt':\n%s\n\n", receivedStream);
    printf("Total Received Stuffed Frame Size: %d bits\n\n", (int)strlen(receivedStream));
}

void BitUnstuffing(char receivedStream[], char unstuffedStream[])
{
    printf("=====================================================================\n");
    printf("                   PPP FRAME BIT UNSTUFFING                          \n");
    printf("=====================================================================\n");

    int totalLen = (int)strlen(receivedStream);
    
    // Drop the starting flag (first 8 bits) and trailing flag (last 8 bits)
    int payloadStart = 8;
    int payloadEnd = totalLen - 8;
    
    int outIdx = 0;
    int consecutiveOnes = 0;

    for (int i = payloadStart; i < payloadEnd; i++)
    {
        char currentBit = receivedStream[i];
        unstuffedStream[outIdx++] = currentBit;

        if (currentBit == '1')
        {
            consecutiveOnes++;
            if (consecutiveOnes == 5)
            {
                // The next bit is a stuffed '0'—skip past it!
                i++; 
                consecutiveOnes = 0;
            }
        }
        else
        {
            consecutiveOnes = 0;
        }
    }
    unstuffedStream[outIdx] = '\0';

    printf("[Receiver] Clean Unstuffed (Headers + Data Payload) Stream:\n%s\n\n", unstuffedStream);
    printf("Total Unstuffed Stream Size: %d bits\n\n", (int)strlen(unstuffedStream));
}

void ReceiverProcess(char unstuffedStream[])
{
    printf("=====================================================================\n");
    printf("                    RECEIVER PACKET PARSING                          \n");
    printf("=====================================================================\n");

    int bitOffset = 0;

    // Slice headers directly out of the unstuffed data pool
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
    
    int rawMessageBits = (int)strlen(unstuffedStream) - bitOffset;
    printf("  [+] Pure Decapsulated Message Data: %d bits\n", rawMessageBits);
    printf("---------------------------------------------------------------------\n");

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

    FILE *fOut = fopen("out.txt", "w");
    if (fOut != NULL)
    {
        fprintf(fOut, "%s", finalMessageText);
        fclose(fOut);
        printf("Success: Decoded message written completely to 'out.txt'!\n");
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

int readInputFile(char fileMessage[])
{
    FILE *filePtr = fopen("inp.txt", "r");
    if (filePtr == NULL) 
    {
        printf("\nError: Could not open 'inp.txt'. Creating a demo file for you.\n");
        FILE *fWrite = fopen("inp.txt", "w");
        fprintf(fWrite, "hi hello");
        fclose(fWrite);
        filePtr = fopen("inp.txt", "r");
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

    // Format matches: SrcMAC + DestMAC + NetworkStream(SrcIP + DestIP + Ports + Data)
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
    sscanf(macStr, "%x:%x:%x:%x:%x:%x", &m1, &m2, &m3, &m4, &m5, &m6);
    char temp[9]; outputStr[0] = '\0';
    charToBinaryString((char)m1, temp); strcat(outputStr, temp);
    charToBinaryString((char)m2, temp); strcat(outputStr, temp);
    charToBinaryString((char)m3, temp); strcat(outputStr, temp);
    charToBinaryString((char)m4, temp); strcat(outputStr, temp);
    charToBinaryString((char)m5, temp); strcat(outputStr, temp);
    charToBinaryString((char)m6, temp); strcat(outputStr, temp);
}