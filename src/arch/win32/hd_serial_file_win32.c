#ifdef MS_WIN32
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>


static char serials[16][10]={"\\\\.\\COM1",  "\\\\.\\COM2",  "\\\\.\\COM3",  "\\\\.\\COM4",
                       "\\\\.\\COM5",  "\\\\.\\COM6",  "\\\\.\\COM7",  "\\\\.\\COM8",
                       "\\\\.\\COM9",  "\\\\.\\COM10", "\\\\.\\COM11", "\\\\.\\COM12",
                       "\\\\.\\COM13", "\\\\.\\COM14", "\\\\.\\COM15", "\\\\.\\COM16"};

typedef struct
{
    HANDLE portHandle;
    DCB options;
} serial_handle_t;

static const uint32_t baudSelect[] = {
    50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200,
    38400, 57600, 115200};


serialError_t SerialOpen(serialPort_t *port, char *name)
{
    HANDLE fd;

    /* open port */
    fd = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                    0, 0);

    /* failed to open */
    if (fd == INVALID_HANDLE_VALUE)
    {
        port->portHandle = 0;
        return SERIAL_FAIL;
    }

    /* return handle to port */
    port->portHandle = fd;

    /* save settings of port */
    GetCommState(fd, &(port->options));

    return SERIAL_OK;
}


serialError_t SerialClose(serialPort_t *port)
{
    /* restore port settings */
    SetCommState(port->portHandle, &(port->options));
    
    /* close the port */
    if (!CloseHandle(port->portHandle))
    {
        return SERIAL_FAIL;
    }

    return SERIAL_OK;
}


serialError_t SerialSettings(serialPort_t *port, uint32_t baud, char parity,
                             uint8_t dataBits, uint8_t stopBits,
                             int32_t timeout_tenths)
{
    DCB dcb;
    char dcbSz[50];
    COMMTIMEOUTS cmt;
    int32_t numBaud;
    int32_t i;

    /* clear dcb and set length */
    FillMemory(&dcb, sizeof(dcb), 0);
    dcb.DCBlength = sizeof(dcb);
    
    /* check baud is valid */
    numBaud = sizeof(baudSelect) / sizeof(uint32_t);
    for (i = 0; i < numBaud; i++)
    {
        if (baud == baudSelect[i])
        {
            break; 
        }
    }
    if (i == numBaud)
    {
        return SERIAL_BAD_BAUD;
    }

    /* check parity */
    switch (parity)
    {
        case 'n':
        case 'o':
        case 'e':
            break;

        default:
            return SERIAL_BAD_PARITY;
    }

    /* check data bits */
    if ((dataBits < 5) || (dataBits > 8))
    {
        return SERIAL_BAD_DATABITS;
    }

    /* check stop bits */
    if ((stopBits < 1) || (stopBits > 2))
    {
        return SERIAL_BAD_STOPBITS;
    }

    /* build dcb */
    sprintf(dcbSz, "%d,%c,%d,%d", baud, parity, dataBits, stopBits);
    if (!BuildCommDCB(dcbSz, &dcb))
    {
        return SERIAL_FAIL;
    }

    /* set dcb to serial port */
    if (!SetCommState(port->portHandle, &dcb))
    {
        return SERIAL_FAIL;
    }

    /* set input and output bufffer sizes */
    if (!SetupComm(port->portHandle, 1024, 1024))
    {
        return SERIAL_FAIL;
    }

    /* set timeouts */
    /* special case of timeout being 0, return whats in buffer */
    if (timeout_tenths == 0)
    {
        cmt.ReadIntervalTimeout = MAXDWORD;
        cmt.ReadTotalTimeoutMultiplier = 0;
        cmt.ReadTotalTimeoutConstant = 0;
        cmt.WriteTotalTimeoutMultiplier = 0;
        cmt.WriteTotalTimeoutConstant = MAXDWORD;
    }
    else
    {
        cmt.ReadIntervalTimeout = MAXDWORD;
        cmt.ReadTotalTimeoutMultiplier = MAXDWORD;
        cmt.ReadTotalTimeoutConstant = timeout_tenths * 10;
        cmt.WriteTotalTimeoutMultiplier = 0;
        cmt.WriteTotalTimeoutConstant = timeout_tenths * 10;
    }

    if (!SetCommTimeouts(port->portHandle, &cmt))
    {
        return SERIAL_FAIL;
    }

    return SERIAL_OK;
}


serialError_t SerialRead(serialPort_t *port, uint8_t *data, int32_t size, int32_t *bytesRead)
{
    unsigned long readCount = 0;

    *bytesRead = 0;

    while (*bytesRead != size)
    {
        readCount = -1;
        
        /* get bytes from port */
        ReadFile(port->portHandle, data + *bytesRead, size - *bytesRead, &readCount, NULL);

        /* if no bytes read timeout has occured */
        if (readCount < 1)
        {
            return SERIAL_TIMEOUT;
        }

        *bytesRead += readCount;
    }

    return SERIAL_OK;
}


serialError_t SerialWrite(serialPort_t *port, uint8_t *data, int32_t size, int32_t *bytesWritten)
{
    /* write bytes */
    *bytesWritten = -1;
    WriteFile(port->portHandle, data, size, (unsigned long *)bytesWritten, NULL);

    /* check all bytes were written */
    if (*bytesWritten != size)
    {
        return SERIAL_WRITE_INCOMPLETE;
    }
    
    return SERIAL_OK;
}


serialError_t SerialEnumerate(void)
{
    return SERIAL_NOT_IMPLEMENTED;
}

#endif /*#MS_WIN32*/
