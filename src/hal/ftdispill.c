#include <stdio.h>
#include <unistd.h>
#include <ftd2xx.h>
#define DEBUG_ENABLE
#define MAXLEN	64


#include "debug.h"

BYTE byOutputBuffer[MAXLEN];   	// Buffer to hold MPSSE commands and data
                              	//     to be sent to the FT2232H
BYTE byInputBuffer[MAXLEN];     	// Buffer to hold data read from the FT2232H
DWORD dwNumBytesToSend = 0;     // Index to the output buffer
DWORD dwNumBytesSent = 0;       // Count of actual bytes sent - used with FT_Write
DWORD dwNumBytesToRead = 0;     // Number of bytes available to read
                                //     in the driver's input buffer
DWORD dwNumBytesRead = 0;       // Count of actual bytes read - used with FT_Read


int spi_init(FT_HANDLE *dev, char *descr, const char *serial)
{
        FT_STATUS ftStatus;             // Result of each D2XX call
        DWORD dwNumDevs;                // The number of devices
	DWORD Count;

        DWORD dwClockDivisor = 0x05DB;  // Value of clock divisor, SCL Frequency =
                                        //60/((1+0x05DB)*2) (MHz) = 1Mhz

//	strncpy(tmpserial,serial,MAXLEN);
	// -----------------------------------------------------------
        // Does an FTDI device exist?
        // -----------------------------------------------------------
        DBG(MSG_INFO,"Checking for FTDI devices...\n");
        ftStatus = FT_CreateDeviceInfoList(&dwNumDevs); // Get the number of FTDI devices
        if (ftStatus != FT_OK) // Did the command execute OK?
        {
		DBG(MSG_ERR,"Error in getting the number of devices\n");
                return 1; //Exit with error
        }
        if (dwNumDevs < 1) // Exit if we don't see any
        {
                DBG(MSG_WARN, "There are no FTDI devices installed\n");
                return 1; // Exit with error
        }
        DBG(MSG_INFO, "%d devices found\n", dwNumDevs);

        // -----------------------------------------------------------
        // Open the port - For this application note, we'll assume the first device is a
        // FT2232H or FT4232H. Further checks can be made against the device
        // descriptions, locations, serial numbers, etc. before opening the port.
        // -----------------------------------------------------------
        DBG(MSG_DEBUG, "Opening device with serial %s and check it has the MPSSE\n", serial);
        ftStatus = FT_OpenEx((void*)serial, FT_OPEN_BY_SERIAL_NUMBER, dev);
        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Open Failed with error %d (Is ftdi_sio or ubserial loaded?)\n", ftStatus);
                return 1; // Exit with error
        }

        // Configure port parameters
        DBG(MSG_DEBUG, "Configuring port for MPSSE use...\n");
        ftStatus |= FT_ResetDevice(*dev); //Reset USB device
        //Purge USB receive buffer first by reading out all old data from FT2232H receive buffer
        ftStatus |= FT_GetQueueStatus(*dev, &dwNumBytesToRead);
        // Get the number of bytes in the FT2232H
        // receive buffer
        if ((ftStatus == FT_OK) && (dwNumBytesToRead > 0))
                FT_Read(*dev, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);
                //Read out the data from FT2232H receive buffer

        ftStatus |= FT_SetUSBParameters(*dev, 65536, 65535);        //Set USB request transfer sizes to 64K
        ftStatus |= FT_SetChars(*dev, 0, 0, 0, 0);                  //Disable event and error characters
        ftStatus |= FT_SetTimeouts(*dev, 0, 5000);                  //Sets the read and write timeouts in millisec$
        ftStatus |= FT_SetLatencyTimer(*dev, 16);                   //Set the latency timer to 1mS (default is 16m$
        ftStatus |= FT_SetFlowControl(*dev, FT_FLOW_RTS_CTS, 0x00, 0x00);   //Turn on flow control to synchronize $
        ftStatus |= FT_SetBitMode(*dev, 0x0, 0x00);                 //Reset controller
        ftStatus |= FT_SetBitMode(*dev, 0x0, 0x02);                 //Enable MPSSE mode
        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error in initializing the MPSSE %d\n", ftStatus);
                
              	FT_SetBitMode(dev, 0x0, 0x00); // Reset the port to disable MPSSE
		FT_Close(dev); // Close the USB port

                return 1;
                // Exit with error
        }

        usleep(500000); // Wait for all the USB stuff to complete and work

        // -----------------------------------------------------------
        // Synchronize the MPSSE by sending a bogus opcode (0xAB),
        // The MPSSE will respond with "Bad Command" (0xFA) followed by
        // the bogus opcode itself.
        // -----------------------------------------------------------
        byOutputBuffer[dwNumBytesToSend++] = 0xAB; //Add bogus command ‘0xAB’ to the queue
        ftStatus = FT_Write(*dev, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);

        // Send off the BAD command
        dwNumBytesToSend = 0;

        // Reset output buffer pointer
        do
        {
                ftStatus = FT_GetQueueStatus(*dev, &dwNumBytesToRead);
                // Get the number of bytes in the device input buffer
        }
        while ((dwNumBytesToRead == 0) && (ftStatus == FT_OK)); //or Timeout

        int bCommandEchod = 0;

        ftStatus = FT_Read(*dev, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);
        //Read out the data from input buffer

        for (Count = 0; Count <dwNumBytesRead - 1; Count++)
        //Check if Bad command and echo command are received
        {
				printf("%d : %x ",Count, byInputBuffer[Count]);
                if ((byInputBuffer[Count] == 0xFA) && (byInputBuffer[Count+1] == 0xAB))
                {
                        bCommandEchod = 1;
                        break;
                }
        }
        if (bCommandEchod == 0)
        {
                DBG(MSG_ERR, "Error in synchronizing the MPSSE\n");
                FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
		FT_Close(*dev); // Close the USB port

                return 1; // Exit with error
        }

        // -----------------------------------------------------------
        // Configure the MPSSE settings for SPI
        // Multple commands can be sent to the MPSSE with one FT_Write
        // -----------------------------------------------------------

        dwNumBytesToSend = 0;   // Start with a fresh index

        // Set up the Hi-Speed specific commands for the FTx232H
        byOutputBuffer[dwNumBytesToSend++] = 0x8A;
        // Use 60MHz master clock (disable divide by 5)

        byOutputBuffer[dwNumBytesToSend++] = 0x97;

        ftStatus = FT_Write(*dev, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent); // Send off the HS-specific commands

        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d setting high-speed MPSSE\n", ftStatus);
                
              	FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
		FT_Close(*dev); // Close the USB port

                return 1;
                // Exit with error
        }

        dwNumBytesToSend = 0;   // Reset output buffer pointer
        // Set TCK frequency
        // TCK = 60MHz /((1 + [(1 +0xValueH*256) OR 0xValueL])*2)
        byOutputBuffer[dwNumBytesToSend++] = '\x86'; //Command to set clock divisor
        byOutputBuffer[dwNumBytesToSend++] = dwClockDivisor & 0xFF; //Set 0xValueL of clock divisor
        byOutputBuffer[dwNumBytesToSend++] = (dwClockDivisor >> 8) & 0xFF; //Set 0xValueH of clock divisor

        ftStatus = FT_Write(*dev, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent); // Send off the clock divisor commands
        dwNumBytesToSend = 0; // Reset output buffer pointer

        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d setting TCK frequency\n", ftStatus);

              	FT_SetBitMode(dev, 0x0, 0x00); // Reset the port to disable MPSSE
		FT_Close(dev); // Close the USB port
                return 1;
                // Exit with error
        }


        // Set initial states of the MPSSE interface
        // -low byte, both pin directions and output values
        // Pin name     Signal  Direction       Config  Initial State   Config
        // ADBUS0       TCK/SK  output          1       high            1
        // ADBUS1       TDI/DO  output          1       low             0
        // ADBUS2       TDO/DI  input           0                       0
        // ADBUS3       TMS/CS  output          1       high            1
        // ADBUS4       GPIOL0  input           0                       0 // Interrupt request
        // ADBUS5       GPIOL1  output          1       low             0
        // ADBUS6       GPIOL2  output          1       high            1
        // ADBUS7       GPIOL3  output          1       high            1

        byOutputBuffer[dwNumBytesToSend++] = 0x80;      // Configure data bits low-byte of MPSSE port
        byOutputBuffer[dwNumBytesToSend++] = 0xC8;      // Initial state config above
        byOutputBuffer[dwNumBytesToSend++] = 0xEB;      // Direction config above
        ftStatus = FT_Write(*dev, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);

        // Send off the low GPIO config commands
        dwNumBytesToSend = 0; // Reset output buffer pointer
        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d setting I/O pins\n", ftStatus);
              	FT_SetBitMode(dev, 0x0, 0x00); // Reset the port to disable MPSSE
              	FT_Close(dev); // Close the USB port

                return 1;
                // Exit with error
        }

        usleep(20000);	// RFM22B needs 16.8ms to start up

	DBG(MSG_DEBUG, "Init completed\n");
	return 0;
}

int spi_setCS(FT_HANDLE *dev, int level)
{
        FT_STATUS ftStatus;             // Result of each D2XX call
        dwNumBytesToSend=0;

        // Set initial states of the MPSSE interface
        // -low byte, both pin directions and output values
        // Pin name     Signal  Direction       Config  Initial State   Config
        // ADBUS0       TCK/SK  output          1       high            1
        // ADBUS1       TDI/DO  output          1       low             0
        // ADBUS2       TDO/DI  input           0                       0
        // ADBUS3       TMS/CS  output          1       high            1
        // ADBUS4       GPIOL0  input           0                       0
        // ADBUS5       GPIOL1  output          1       low             0
        // ADBUS6       GPIOL2  output          1       high            1
        // ADBUS7       GPIOL3  output          1       high            1

        // Lower CS
        byOutputBuffer[dwNumBytesToSend++] = 0x80;      // Configure data bits low-byte of MPSSE port

	if (level==0)
	{
        	byOutputBuffer[dwNumBytesToSend++] = 0xC0;      // Low level (Active)
		DBG(MSG_DEBUG, "Setting CS Low \n");
	}
	else
	{
		byOutputBuffer[dwNumBytesToSend++] = 0xC8;	// High level (Idle)
		DBG(MSG_DEBUG, "Setting CS High \n");
	}
        byOutputBuffer[dwNumBytesToSend++] = 0xEB;      // Direction config above
        ftStatus = FT_Write(*dev, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);

        // Send off the low GPIO config commands
        dwNumBytesToSend = 0; // Reset output buffer pointer

        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d setting CS pin\n", ftStatus);
                FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
                FT_Close(*dev); // Close the USB port

                return 1;
                // Exit with error
        }
	return 0;
}

int spi_getInt(FT_HANDLE *dev)
{
        FT_STATUS ftStatus;             // Result of each D2XX call
        dwNumBytesToSend=0;

        // Set initial states of the MPSSE interface
        // -low byte, both pin directions and output values
        // Pin name     Signal  Direction       Config  Initial State   Config
        // ADBUS0       TCK/SK  output          1       high            1
        // ADBUS1       TDI/DO  output          1       low             0
        // ADBUS2       TDO/DI  input           0                       0
        // ADBUS3       TMS/CS  output          1       high            1
        // ADBUS4       GPIOL0  input           0                       0 // INT
        // ADBUS5       GPIOL1  output          1       low             0
        // ADBUS6       GPIOL2  output          1       high            1
        // ADBUS7       GPIOL3  output          1       high            1

        // Lower CS
        byOutputBuffer[dwNumBytesToSend++] = 0x81;      // Configure data bits low-byte of MPSSE port

        ftStatus = FT_Write(*dev, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);

        // Send off the low GPIO config commands
        dwNumBytesToSend = 0; // Reset output buffer pointer

	usleep(100000);

	// Check the receive buffer - there should be one byte
	ftStatus |= FT_GetQueueStatus(*dev, &dwNumBytesToRead); 
	// Get the number of bytes in the 
	// FT2232H receive buffer
	ftStatus |= FT_Read(*dev, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead); 


        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d reading INT pin\n", ftStatus);
                FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
                FT_Close(*dev); // Close the USB port

                return -1;
                // Exit with error
        }


        return byInputBuffer[0]&0x10;
}

int spi_trans(FT_HANDLE *dev, unsigned int len, char *rxbuf, char *txbuf)
{
        FT_STATUS ftStatus;             // Result of each D2XX call
	unsigned int count;
        // Now repeat the transmission with the send and receive op-code in place of transmit-only

	dwNumBytesToSend=0;

        // Data Transmit, with receive
        byOutputBuffer[dwNumBytesToSend++] = 0x31;
        // Output on falling clock, input on rising clock
        // MSB first, clock a number of bytes out

        byOutputBuffer[dwNumBytesToSend++] = len-1; // Length L (length of 0 will send 1 byte)
        byOutputBuffer[dwNumBytesToSend++] = 0x00; // Length H
        // Length = 0x0001 + 1

        ftStatus = FT_GetQueueStatus(*dev, &dwNumBytesToRead);
        // Get the number of bytes in
        // the FT2232H receive buffer
        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d polling for data\n", ftStatus);
                FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
                FT_Close(*dev); // Close the USB port

                return 1;
                // Exit with error
        }
        else
        {
                DBG(MSG_DEBUG, "%d lost bytes in buffer\n", dwNumBytesToRead);
        }


	if (len>(MAXLEN-3))
 	{
		DBG(MSG_WARN, "To much data. Truncating to %d bytes. Change MAXLEN define.\n", MAXLEN-3);
	}

	for (count=0; ((count<len)&&(count<=(MAXLEN-3))); count++)
	{
        	byOutputBuffer[dwNumBytesToSend++]=txbuf[count];
	}
        ftStatus = FT_Write(*dev, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);

        dwNumBytesToSend = 0;
        // Reset output buffer pointer

        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d sending data\n", ftStatus);
                FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
                FT_Close(*dev); // Close the USB port

                return 1;
                // Exit with error
        }

	DBG(MSG_DEBUG, "%d bytes send\n", dwNumBytesSent);
        usleep(100000);
        // Wait for data to be transmitted and status
        // to be returned by the device driver
        // -see latency timer above
        // Check the receive buffer - it should contain the looped-back data

        ftStatus = FT_GetQueueStatus(*dev, &dwNumBytesToRead);
        // Get the number of bytes in
        // the FT2232H receive buffer
        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d polling for data\n", ftStatus);
                FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
                FT_Close(*dev); // Close the USB port

                return 1;
                // Exit with error
        }
	else
	{
		DBG(MSG_DEBUG, "%d bytes in buffer\n", dwNumBytesToRead);
	}

        ftStatus=FT_Read(*dev, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);

        if (ftStatus != FT_OK)
        {
                DBG(MSG_ERR, "Error %d receiving data\n", ftStatus);
                FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
                FT_Close(*dev); // Close the USB port

                return 1;
                // Exit with error
        }

	DBG(MSG_DEBUG, "%d bytes received\n", dwNumBytesRead);

        for(count = 0; count < dwNumBytesRead; count++)
        {
		rxbuf[count]=byInputBuffer[count];
        }

	return dwNumBytesSent;
}

void spi_close(FT_HANDLE *dev)
{
	DBG(MSG_DEBUG, "Closing SPI port\n");
    	FT_SetBitMode(*dev, 0x0, 0x00); // Reset the port to disable MPSSE
     	FT_Close(*dev); // Close the USB port
}
