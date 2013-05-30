//----------------------------------------------------------------------------
//	hd_com_api.h
//  author: Joy.you
// 	2013-05-08
//
//
//
//----------------------------------------------------------------------------

#ifndef _HD_COM_API_H_
#define _HD_COM_API_H_

//----------------------------------------------------------------------------
//

#include <hd_plat_base.h>
#include <hd_thread_api.h>

//----------------------------------------------------------------------------
//	comm mask
#define			COM_EV_BREAK			(1<<0)	//	A break was detected on input
#define			COM_EV_CTS				(1<<1)	//	The CTS (clear-to-send) signal changed state
#define			COM_EV_DSR				(1<<2)	//	The DSR (data-set-ready) signal changed state
#define			COM_EV_ERR				(1<<3)	//	A line-status error occurred. Line-status errors are CE_FRAME, CE_OVERRUN, and CE_RXPARITY
#define			COM_EV_RING				(1<<4)	//	A ring indicator was detected
#define			COM_EV_RLSD				(1<<5)	//	The RLSD (receive-line-signal-detect) signal changed state
#define			COM_EV_RXCHAR			(1<<6)	//	A e_int8acter was received and placed in the input buffer
#define			COM_EV_RXFLAG			(1<<7)	//	The event e_int8acter was received and placed in the input buffer. The event e_int8acter is specified in the device’s DCB structure, which is applied to a serial port by using the SetCommState function
#define			COM_EV_TXEMPTY		    (1<<8)	//	The last e_int8acter in the output buffer was sent
//----------------------------------------------------------------------------
//	comm purge flag
#define			COM_PURGE_TXABORT		(1<<0)	//	Terminates all outstanding overlapped write operations and returns immediately, even if the write operations have not been completed.
#define			COM_PURGE_RXABORT		(1<<1)	//	Terminates all outstanding overlapped read operations and returns immediately, even if the read operations have not been completed.
#define			COM_PURGE_TXCLEAR		(1<<2)	//	Clears the output buffer (if the device driver has one).
#define			COM_PURGE_RXCLEAR		(1<<3)	//	Clears the input buffer (if the device driver has one).

//----------------------------------------------------------------------------
//	com device control block struct definition
typedef struct
{
	e_uint32 DCBlength; /* sizeof(DCB)                     */
	e_uint32 BaudRate; /* Baudrate at which running       */
	e_uint32 fBinary :1; /* Binary Mode (skip EOF check)    */
	e_uint32 fParity :1; /* Enable parity checking          */
	e_uint32 fOutxCtsFlow :1; /* CTS handshaking on output       */
	e_uint32 fOutxDsrFlow :1; /* DSR handshaking on output       */
	e_uint32 fDtrControl :2; /* DTR Flow control                */
	e_uint32 fDsrSensitivity :1; /* DSR Sensitivity              */
	e_uint32 fTXContinueOnXoff :1; /* Continue TX when Xoff sent */
	e_uint32 fOutX :1; /* Enable output X-ON/X-OFF        */
	e_uint32 fInX :1; /* Enable input X-ON/X-OFF         */
	e_uint32 fErrorChar :1; /* Enable Err Replacement          */
	e_uint32 fNull :1; /* Enable Null stripping           */
	e_uint32 fRtsControl :2; /* Rts Flow control                */
	e_uint32 fAbortOnError :1; /* Abort all reads and writes on Error */
	e_uint32 fDummy2 :17; /* Reserved                        */
	e_uint16 wReserved; /* Not currently used              */
	e_uint16 XonLim; /* Transmit X-ON threshold         */
	e_uint16 XoffLim; /* Transmit X-OFF threshold        */
	e_uint8 ByteSize; /* Number of bits/e_uint8, 4-8        */
	e_uint8 Parity; /* 0-4=None,Odd,Even,Mark,Space    */
	e_uint8 StopBits; /* 0,1,2 = 1, 1.5, 2               */
	char XonChar; /* Tx and Rx X-ON character        */
	char XoffChar; /* Tx and Rx X-OFF character       */
	char ErrorChar; /* Error replacement char          */
	char EofChar; /* End of Input character          */
	char EvtChar; /* Received Event character        */
	e_uint16 wReserved1; /* Fill for now.                   */
} com_dcb_t;

//----------------------------------------------------------------------------
//	comm device control block struct definition
typedef struct
{
	void *priv; //	com handle
	ethread_t *thread; //	attach thread
	com_dcb_t dcb; //	com dcb data

	e_uint32 state; //	com state: open or not
	e_uint32 ready; //	com state read to do operation
	e_uint32 mask; //	com monitor mask
	e_int8 name[16]; //	com name: com1 ......
} com_t;

/*	reference to msdn
 DCBlength
 Specifies the DCB structure length, in e_uint8s.
 BaudRate
 Specifies the baud rate at which the communication device operates. It is an actual baud rate value, or one of the following baud rate indexes: CBR_110 CBR_19200
 CBR_300 CBR_38400
 CBR_600 CBR_56000
 CBR_1200 CBR_57600
 CBR_2400 CBR_115200
 CBR_4800 CBR_128000
 CBR_9600 CBR_256000
 CBR_14400


 fBinary
 Specifies if binary mode is enabled. The Win32 API does not support nonbinary mode transfers, so this member must be TRUE. Using FALSE will not work.
 fParity
 Specifies if parity checking is enabled. If this member is TRUE, parity checking is performed and errors are reported.
 fOutxCtsFlow
 Specifies if the CTS (clear-to-send) signal is monitored for output flow control. If this member is TRUE and CTS is turned off, output is suspended until CTS is sent again.
 fOutxDsrFlow
 Specifies if the DSR (data-set-ready) signal is monitored for output flow control. If this member is TRUE and DSR is turned off, output is suspended until DSR is sent again.
 fDtrControl
 Specifies the DTR (data-terminal-ready) flow control. This member can be one of the following values: Value Description
 DTR_CONTROL_DISABLE Disables the DTR line when the device is opened and leaves it disabled.
 DTR_CONTROL_ENABLE Enables the DTR line when the device is opened and leaves it on.
 DTR_CONTROL_HANDSHAKE Enables DTR handshaking. If handshaking is enabled, it is an error for the application to adjust the line by using the EscapeCommFunction function.


 fDsrSensitivity
 Specifies if the communications driver is sensitive to the state of the DSR signal. If this member is TRUE, the driver ignores any e_uint8s received, unless the DSR modem input line is high.
 fTXContinueOnXoff
 Specifies if transmission stops when the input buffer is full and the driver has transmitted the Xoffe_int8 e_int8acter. If this member is TRUE, transmission continues after the input buffer has come within XoffLim e_uint8s of being full and the driver has transmitted the Xoffe_int8 e_int8acter to stop receiving e_uint8s. If this member is FALSE, transmission does not continue until the input buffer is within XonLim e_uint8s of being empty and the driver has transmitted the Xone_int8 e_int8acter to resume reception.
 fOutX
 Specifies if XON/XOFF flow control is used during transmission. If this member is TRUE, transmission stops when the Xoffe_int8 e_int8acter is received and starts again when the Xone_int8 e_int8acter is received.
 fInX
 Specifies if XON/XOFF flow control is used during reception. If this member is TRUE, the Xoffe_int8 e_int8acter is sent when the input buffer comes within XoffLim e_uint8s of being full, and the Xone_int8 e_int8acter is sent when the input buffer comes within XonLim e_uint8s of being empty.
 fErrore_int8
 Specifies if e_uint8s received with parity errors are replaced with the e_int8acter specified by the Errore_int8 member. If this member is TRUE and the fParity member is TRUE, replacement occurs.
 fNull
 Specifies if null e_uint8s are discarded. If this member is TRUE, null e_uint8s are discarded when received.
 fRtsControl
 Specifies the RTS (request-to-send) flow control. If this value is zero, the default is RTS_CONTROL_HANDSHAKE. This member can be one of the following values: Value Description
 RTS_CONTROL_DISABLE Disables the RTS line when the device is opened and leaves it disabled.
 RTS_CONTROL_ENABLE Enables the RTS line when the device is opened and leaves it on.
 RTS_CONTROL_HANDSHAKE Enables RTS handshaking. The driver raises the RTS line when the “type-ahead” (input) buffer is less than one-half full and lowers the RTS line when the buffer is more than three-quarters full. If handshaking is enabled, it is an error for the application to adjust the line by using the EscapeCommFunction function.
 RTS_CONTROL_TOGGLE Specifies that the RTS line will be high if e_uint8s are available for transmission. After all buffered e_uint8s have been sent, the RTS line will be low.


 fAbortOnError
 Specifies if read and write operations are terminated if an error occurs. If this member is TRUE, the driver terminates all read and write operations with an error status if an error occurs. The driver will not accept any further communications operations until the application has acknowledged the error by calling the ClearCommError function.
 fDummy2
 Reserved; do not use.
 wReserved
 Not used; set to zero.
 XonLim
 Specifies the minimum number of e_uint8s accepted in the input buffer before the XON e_int8acter is sent.
 XoffLim
 Specifies the maximum number of e_uint8s accepted in the input buffer before the XOFF e_int8acter is sent. The maximum number of e_uint8s accepted is calculated by subtracting this value from the size, in e_uint8s, of the input buffer.
 e_uint8Size
 Specifies the number of bits in the e_uint8s transmitted and received.
 Parity
 Specifies the parity scheme to be used. It is one of the following values: Value Description
 EVENPARITY Even
 MARKPARITY Mark
 NOPARITY No parity
 ODDPARITY Odd
 SPACEPARITY Space


 StopBits
 Specifies the number of stop bits to be used. It is one of the following values: Value Description
 ONESTOPBIT 1 stop bit
 ONE5STOPBITS 1.5 stop bits
 TWOSTOPBITS 2 stop bits


 Xone_int8
 Specifies the value of the XON e_int8acter for both transmission and reception.
 Xoffe_int8
 Specifies the value of the XOFF e_int8acter for both transmission and reception.
 Errore_int8
 Specifies the value of the e_int8acter used to replace e_uint8s received with a parity error.
 Eofe_int8
 Specifies the value of the e_int8acter used to signal the end of data.
 Evte_int8
 Specifies the value of the e_int8acter used to signal an event.
 wReserved1
 Reserved; do not use.
 */
//----------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//----------------------------------------------------------------------------
//	open/close/state comm device
e_uint8 DEV_EXPORT Comm_Open(com_t *com, const char* com_name, const int bandrate);
void DEV_EXPORT Comm_Close(com_t *com);
e_uint8 DEV_EXPORT Comm_State(com_t *com);

//	attach and detach thread object
ethread_t DEV_EXPORT *Comm_Attach(com_t *com, ethread_t *thread);
void DEV_EXPORT Comm_Detach(com_t *com);

//	get/set comm state
e_bool DEV_EXPORT Comm_GetData(com_t *com, com_dcb_t *dcb);
e_bool DEV_EXPORT Comm_SetData(com_t *com, const com_dcb_t *dcb);

//	get/set comm monitored mask
e_uint32 DEV_EXPORT Comm_SetMask(com_t *com, e_uint32 mask);
e_uint32 DEV_EXPORT Comm_GetMask(com_t *com);

//	clear comm output/input
e_bool DEV_EXPORT Comm_Purge(com_t *com, e_uint32 flag);

//	read/write comm data
e_uint32 DEV_EXPORT Comm_Read(com_t *com, e_uint8 *buffer, e_uint32 blen);
e_uint32 DEV_EXPORT Comm_ReadLine( com_t *com, e_uint8 *buffer, e_uint32 blen, e_uint8 *endchar);
e_uint32 DEV_EXPORT Comm_Write(com_t *com, unsigned char *buffer,
		unsigned long len);

//----------------------------------------------------------------------------
//
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif	//	_HD_COM_API_H_
//----------------------------------------------------------------------------
// EOF emap_com_api.h
