// 
// Copyright (c) 2017 Y2 Corporation
// 
// All rights reserved. 
// 
// MIT License
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "ftd2xx.h"
#include "libft4222.h"

static FT4222_STATUS readGPIO(FT_HANDLE ftHandle, const uint16  slaveAddr, uint8_t *data)
{
	FT4222_STATUS        ft4222Status;
	uint16               bytesToWrite = 1;
	uint16               bytesWritten = 0;

	uint8_t writeBuffer[2];
	writeBuffer[0] = 1;		// IP1
	writeBuffer[1] = 0x00;
	uint8_t readData;

	ft4222Status = FT4222_I2CMaster_WriteEx(ftHandle,
		slaveAddr,
		START,
		writeBuffer,
		1,
		&bytesWritten);
	if (FT4222_OK != ft4222Status) {
		printf("FT4222_I2CMaster_Write 1 failed (error %d)\n",
			(int)ft4222Status);
		return ft4222Status;
	}

	if (bytesWritten != 1) {
		printf("FT4222_I2CMaster_Write wrote %u of %u bytes.\n",
			bytesWritten,
			bytesToWrite);
	}

	ft4222Status = FT4222_I2CMaster_ReadEx(ftHandle,
		slaveAddr,
		Repeated_START | STOP,
		&readData,
		1,
		&bytesWritten);
	if (FT4222_OK != ft4222Status) {
		printf("FT4222_I2CMaster_Read 1 failed (error %d)\n",
			(int)ft4222Status);
		return ft4222Status;
	}

	if (bytesWritten != bytesToWrite) {
		printf("FT4222_I2CMaster_Read wrote %u of %u bytes.\n",
			bytesWritten,
			bytesToWrite);
	}

	*data = ~readData;
	return ft4222Status;
}

static FT4222_STATUS writeGPIO(FT_HANDLE ftHandle, const uint16  slaveAddr, uint8_t data)
{
	FT4222_STATUS        ft4222Status;
	uint16               bytesToWrite = 1;
	uint16               bytesWritten = 0;

	uint8_t writeBuffer[2];
	writeBuffer[0] = 6;		// IOC0
	writeBuffer[1] = 0x00;

	ft4222Status = FT4222_I2CMaster_Write(ftHandle,
		slaveAddr,
		writeBuffer,
		2,
		&bytesWritten);
	if (FT4222_OK != ft4222Status) {
		printf("FT4222_I2CMaster_Write 1 failed (error %d)\n",
			(int)ft4222Status);
		return ft4222Status;
	}

	if (bytesWritten != 2) {
		printf("FT4222_I2CMaster_Write wrote %u of %u bytes.\n",
			bytesWritten,
			bytesToWrite);
	}

	writeBuffer[0] = 2;		// OP0
	writeBuffer[1] = ~data;

	ft4222Status = FT4222_I2CMaster_Write(ftHandle,
		slaveAddr,
		writeBuffer,
		2,
		&bytesWritten);
	if (FT4222_OK != ft4222Status) {
		printf("FT4222_I2CMaster_Write 1 failed (error %d)\n",
			(int)ft4222Status);
		return ft4222Status;
	}

	if (bytesWritten != 2) {
		printf("FT4222_I2CMaster_Write wrote %u of %u bytes.\n",
			bytesWritten,
			bytesToWrite);
	}

	return ft4222Status;
}

static int exercise4222(DWORD locationId)
{
	int                  success = 0;
	FT_STATUS            ftStatus;
	FT_HANDLE            ftHandle = (FT_HANDLE)NULL;
	FT4222_STATUS        ft4222Status;
	FT4222_Version       ft4222Version;
	const uint16         slaveAddr = 0x50;
	uint16               bytesToRead;
	uint16               bytesRead = 0;
	uint16               bytesToWrite;
	uint16               bytesWritten = 0;
	char                *writeBuffer;
	int                  page;

	uint8_t gpioData;
	uint8_t prevData;

	ftStatus = FT_OpenEx((PVOID)(uintptr_t)locationId,
		FT_OPEN_BY_LOCATION,
		&ftHandle);
	if (ftStatus != FT_OK) {
		printf("FT_OpenEx failed (error %d)\n",
			(int)ftStatus);
		goto exit;
	}

	ft4222Status = FT4222_GetVersion(ftHandle,
		&ft4222Version);
	if (FT4222_OK != ft4222Status) {
		printf("FT4222_GetVersion failed (error %d)\n",
			(int)ft4222Status);
		goto exit;
	}

	printf("Chip version: %08X, LibFT4222 version: %08X\n",
		(unsigned int)ft4222Version.chipVersion,
		(unsigned int)ft4222Version.dllVersion);

	// Configure the FT4222 as an I2C Master
	ft4222Status = FT4222_I2CMaster_Init(ftHandle, 400);
	if (FT4222_OK != ft4222Status) {
		printf("FT4222_I2CMaster_Init failed (error %d)!\n",
			ft4222Status);
		goto exit;
	}

	// Reset the I2CM registers to a known state.
	ft4222Status = FT4222_I2CMaster_Reset(ftHandle);
	if (FT4222_OK != ft4222Status) {
		printf("FT4222_I2CMaster_Reset failed (error %d)!\n",
			ft4222Status);
		goto exit;
	}
	printf("--- Loopback Test ---\n");

	for (;;) {
		ft4222Status = readGPIO(ftHandle, 0x26, &gpioData);
		if (FT4222_OK != ft4222Status) {
			printf("FT4222_ReadGPIO failed (error %d)!\n",
				ft4222Status);
			break;
		}
		if (prevData != gpioData) {
			printf("Data: %02X\n", gpioData);
			prevData = gpioData;
		}

		ft4222Status = writeGPIO(ftHandle, 0x26, gpioData);
		if (FT4222_OK != ft4222Status) {
			printf("writeGPIO failed (error %d)!\n",
				ft4222Status);
			break;
		}
	}
	
exit:
	if (ftHandle != (FT_HANDLE)NULL) {
		(void)FT4222_UnInitialize(ftHandle);
		(void)FT_Close(ftHandle);
	}

	return success;
}

static int testFT4222(void)
{
	FT_STATUS                 ftStatus;
	FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;
	DWORD                     numDevs = 0;
	int                       i;
	int                       retCode = 0;
	int                       found4222 = 0;

	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	if (ftStatus != FT_OK) {
		printf("FT_CreateDeviceInfoList failed (error code %d)\n",
			(int)ftStatus);
		retCode = -10;
		goto exit;
	}

	if (numDevs == 0) {
		printf("No devices connected.\n");
		retCode = -20;
		goto exit;
	}

	/* Allocate storage */
	devInfo = calloc((size_t)numDevs,
		sizeof(FT_DEVICE_LIST_INFO_NODE));
	if (devInfo == NULL) {
		printf("Allocation failure.\n");
		retCode = -30;
		goto exit;
	}

	/* Populate the list of info nodes */
	ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
	if (ftStatus != FT_OK) {
		printf("FT_GetDeviceInfoList failed (error code %d)\n",
			(int)ftStatus);
		retCode = -40;
		goto exit;
	}

	for (i = 0; i < (int)numDevs; i++) {
		unsigned int devType = devInfo[i].Type;
		size_t       descLen;

		if (devType == FT_DEVICE_4222H_0) {
			// In mode 0, the FT4222H presents two interfaces: A and B.
			descLen = strlen(devInfo[i].Description);

			if ('A' == devInfo[i].Description[descLen - 1]) {
				// Interface A may be configured as an I2C master.
				printf("\nDevice %d is interface A of mode-0 FT4222H:\n",
					i);
				printf("  0x%08x  %s  %s\n",
					(unsigned int)devInfo[i].ID,
					devInfo[i].SerialNumber,
					devInfo[i].Description);
				(void)exercise4222(devInfo[i].LocId);
			} else {
				// Interface B of mode 0 is reserved for GPIO.
				printf("Skipping interface B of mode-0 FT4222H.\n");
			}

			found4222++;
		}

		if (devType == FT_DEVICE_4222H_1_2) {
			// In modes 1 and 2, the FT4222H presents four interfaces but
			// none is suitable for I2C.
			descLen = strlen(devInfo[i].Description);

			printf("Skipping interface %c of mode-1/2 FT4222H.\n",
				devInfo[i].Description[descLen - 1]);

			found4222++;
		}

		if (devType == FT_DEVICE_4222H_3) {
			// In mode 3, the FT4222H presents a single interface.  
			// It may be configured as an I2C Master.
			printf("\nDevice %d is mode-3 FT4222H (single Master/Slave):\n",
				i);
			printf("  0x%08x  %s  %s\n",
				(unsigned int)devInfo[i].ID,
				devInfo[i].SerialNumber,
				devInfo[i].Description);
			(void)exercise4222(devInfo[i].LocId);

			found4222++;
		}
	}

	if (!found4222)
		printf("No FT4222 found.\n");

exit:
	free(devInfo);
	return retCode;
}

int main(void)
{
	return testFT4222();
}

