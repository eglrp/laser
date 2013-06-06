#ifndef HD_SERIAL_H
#define HD_SERIAL_H

#include "hd_plat_base.h"

typedef struct serial_handle_t serial_handle;
//----------------------------------------------------------------------------
//	comm device control block struct definition
typedef struct
{
	serial_handle *priv; //	serial private handle

	e_uint32 read_timeout_usec;
	e_uint32 write_timeout_usec;
	e_uint32 state; //	serial state: open or not
	e_int8 name[16]; //	serial name: com1 ......
	e_int32 speed;
} serial_t;

e_int32 DEV_EXPORT Serial_Open(serial_t *port, char *name);
e_int32 DEV_EXPORT Serial_Close(serial_t *port);

/*
串行通信在软件设置里需要做多项设置，最常见的设置包括

波特率（Baud Rate）

奇偶校验（Parity Check）

停止位（Stop Bit）。

波特率（又稱鮑率）：是指从一设备发到另一设备的波特率，即每秒钟多少比特bits per second (bit/s)。
典型的波特率是300, 1200, 2400, 9600, 115200, 19200等bit/s。
一般通信两端设备都要设为相同的波特率，但有些设备也可以设置为自动检测波特率。
奇偶校验（Parity：是用来验证数据的正确性。奇偶校验一般不使用，如果使用，
那么既可以做奇校验（Odd Parity）也可以做偶校验（Even Parity）。
奇偶校验是通过修改每一发送字节（也可以限制发送的字节）来工作的。如果不作奇偶校验，
那么数据是不会被改变的。在偶校验中，因为奇偶校验位会被相应的置1或0（一般是最高位或最低位），
所以数据会被改变以使得所有传送的数位（含字符的各数位和校验位）中“1”的个数为偶数；在奇校验中，
所有传送的数位（含字符的各数位和校验位）中“1”的个数为奇数。
奇偶校验可以用于接受方检查传输是否发送生错误——如果某一字节中“1”的个数发生了错误，
那么这个字节在传输中一定有错误发生。如果奇偶校验是正确的，那么要么没有发生错误要么发生了偶数个的错误。
如果用户选择数据长度为8位，则因为没有多余的比特可被用来作为同比特，因此就叫做“無位元（Non Parity）”。
停止位：是在每个字节传输之后发送的，它用来帮助接受信号方硬件重同步。
*/
/**********************************************************************
Serial_Settings: Set the settings of the serial port interface. Should
be called after the port is opened because otherwise settings
could be lost.
8Parameters:
speed: the desired speed of the serial port
wordSize: the size of the data word in bits
stopBits: the number of stop bits
parity: parity to be used. One of
’n’ - None
’e’ - Even
’o’ - Odd
Returns: boolean indicating success or failure
***********************************************************************/
e_int32 DEV_EXPORT Serial_Settings(serial_t *port, e_uint32 baud, char parity,
                             e_uint8 wordSize, e_uint8 stopBits,
                             e_int32 timeout_tenths);

/*********************************************************************
Serial_Timeouts: set the read and write timeouts of the serial port
Parameters:
readTimeout: The amount of time to wait for a single word
read to timeout
writeTimeout: The amount of time to wait for a single word
write to timeout
Returns: boolean indicating success or failure
注意：使用此函数会对所有的Read，Write产生影响
传小于=0的值，会禁用超时。
**********************************************************************/
e_int32 DEV_EXPORT Serial_Timeouts(serial_t *port,int readTimeout_usec, int writeTimeout_usec);
e_int32 DEV_EXPORT Serial_Select(serial_t *port,e_int32 type,e_int32 timeout_usec);
e_int32 DEV_EXPORT Serial_Read(serial_t *port, e_uint8 *data, e_int32 size);
e_int32 DEV_EXPORT Serial_Write(serial_t *port, e_uint8 *data, e_int32 size);
e_int32 DEV_EXPORT Serial_Enumerate(void);

#endif /* !HD_SERIAL_H */
