#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#define BUFFER_SIZE 256

HANDLE open_port(const char* device, unsigned long baud_rate, unsigned char bit_size, unsigned char parity) {
    HANDLE port = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (port == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    // Flush away any bytes previously read or written.
    BOOL success = FlushFileBuffers(port);
    if (!success)
    {
        printf("Failed to flush serial port");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    // Configure read and write operations to time out after 100 ms.
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    success = SetCommTimeouts(port, &timeouts);
    if (!success)
    {
        printf("Failed to set serial timeouts");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }
    // Set the baud rate and other options.
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);
    dcb.BaudRate = baud_rate;
    dcb.ByteSize = bit_size;
    dcb.Parity = parity;
    dcb.StopBits = ONESTOPBIT;
    success = SetCommState(port, &dcb);
    if (!success)
    {
        printf("Failed to set serial settings");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    // display information
    printf("----------------------------------\n");
    printf("baud rate = %d\n", dcb.BaudRate);
    printf("Parity = %d\n", dcb.Parity);
    printf("Byte Size = %d\n", dcb.ByteSize);
    printf("Stop Bit = %d\n", dcb.StopBits);
    printf("----------------------------------\n");
    return port;
}

int uart_transmit(HANDLE port, unsigned char* TX_buf, unsigned int TX_buf_len) {
    int writtenbytes = 0;
    if (WriteFile(port, TX_buf, TX_buf_len, (LPDWORD)&writtenbytes, NULL)) {
        if (writtenbytes == 0) {
            printf("WriteFile() timed out\n");
            return -1;
        }
    }
    else {
        printf("WriteFile() failed\n");
        return -1;
    }
    printf("%d bytes were written\n", writtenbytes);
    return 0;
}

int uart_receive(HANDLE port, int* RX_buf_len, unsigned char* RX_buf) {
    char readbuf[BUFFER_SIZE];
    int nbbytes = 0;
    
    if (ReadFile(port, &readbuf, sizeof(readbuf)-1, (LPDWORD)&nbbytes, NULL)) {
        if (nbbytes == 0) {
            printf("ReadFile() timed out\n");
            return -1;
        }
    }
    else {
        printf("ReadFile() failed\n");
        return -1;
    }
    int size = sizeof(readbuf);
    *RX_buf_len = nbbytes;
    for (int i = 0; i < size; i++) { //Read code from silab
        RX_buf[i] = readbuf[i];
        
    }
    //*RX_buf_len = 1;//////////**********************////*****
    //RX_buf[0] = readbuf; ///////////******////

    return 0;
}


int main()
{
    const char* device = "\\\\.\\COM6"; // C?ng COM5
    unsigned long baud_rate = 115200;
    unsigned char bit_size = 8;
    unsigned char parity = 0;
 
    int status = -1;
    unsigned char TX_buf[BUFFER_SIZE]="Hello World", RX_buf[BUFFER_SIZE];
    unsigned int TX_buf_len = 0, RX_buf_len = 0;
    HANDLE port = open_port(device, baud_rate, bit_size, parity);
    if (port == INVALID_HANDLE_VALUE) {
        return -1;
    }

    TX_buf_len = sizeof(TX_buf);
    
    // transmit data
    status = uart_transmit(port, TX_buf, TX_buf_len);
    if (status != 0)
        return -1;

    // Receive data
    status = uart_receive(port, &RX_buf_len, RX_buf);
    if (status != 0)
        return -1;

    // Display 
    printf("\nReceived packet include time: ");
    for (int j = 0; j < RX_buf_len; j++) {
        printf("%c", RX_buf[j]);
    }

    // Close the serial port.
    if (!CloseHandle(port))
    {
        printf("CloseHandle() failed\n");
        return -1;
    }
    return 0;
}
