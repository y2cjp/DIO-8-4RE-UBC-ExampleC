// FTDIのサンプル（i2c_master.cpp）をベースにDIO-8/4RE-UBCのデジタル入出力のテストを追加したものです。

/**
 * @file i2c_master.cpp
 *
 * @author FTDI
 * @date 2014-07-01
 *
 * Copyright © 2011 Future Technology Devices International Limited
 * Company Confidential
 *
 * Rivision History:
 * 1.0 - initial version
 */

//------------------------------------------------------------------------------
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

//------------------------------------------------------------------------------
// include FTDI libraries
//
#include "ftd2xx.h"
#include "LibFT4222.h"


std::vector< FT_DEVICE_LIST_INFO_NODE > g_FT4222DevList;

//------------------------------------------------------------------------------
inline std::string DeviceFlagToString(DWORD flags)
{
    std::string msg;
    msg += (flags & 0x1)? "DEVICE_OPEN" : "DEVICE_CLOSED";
    msg += ", ";
    msg += (flags & 0x2)? "High-speed USB" : "Full-speed USB";
    return msg;
}

void ListFtUsbDevices()
{
    FT_STATUS ftStatus = 0;

    DWORD numOfDevices = 0;
    ftStatus = FT_CreateDeviceInfoList(&numOfDevices);

    for(DWORD iDev=0; iDev<numOfDevices; ++iDev)
    {
        FT_DEVICE_LIST_INFO_NODE devInfo;
        memset(&devInfo, 0, sizeof(devInfo));

        ftStatus = FT_GetDeviceInfoDetail(iDev, &devInfo.Flags, &devInfo.Type,
                                        &devInfo.ID, &devInfo.LocId,
                                        devInfo.SerialNumber,
                                        devInfo.Description,
                                        &devInfo.ftHandle);

        if (FT_OK == ftStatus)
        {
            const std::string desc = devInfo.Description;
            if(desc == "FT4222" || desc == "FT4222 A")
            {
                g_FT4222DevList.push_back(devInfo);
            }
        }
    }
}

static FT4222_STATUS readGPIO(FT_HANDLE ftHandle, const uint16  slaveAddr, uint8_t* data)
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

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(int argc, char const *argv[])
{
    ListFtUsbDevices();

    if(g_FT4222DevList.empty()) {
        printf("No FT4222 device is found!\n");
        return 0;
    }

    const FT_DEVICE_LIST_INFO_NODE& devInfo = g_FT4222DevList[0];

    printf("Open Device\n");
    printf("  Flags= 0x%x, (%s)\n", devInfo.Flags, DeviceFlagToString(devInfo.Flags).c_str());
    printf("  Type= 0x%x\n",        devInfo.Type);
    printf("  ID= 0x%x\n",          devInfo.ID);
    printf("  LocId= 0x%x\n",       devInfo.LocId);
    printf("  SerialNumber= %s\n",  devInfo.SerialNumber);
    printf("  Description= %s\n",   devInfo.Description);
    printf("  ftHandle= 0x%x\n",    devInfo.ftHandle);


    FT_HANDLE ftHandle = NULL;

    FT_STATUS ftStatus;
    ftStatus = FT_OpenEx((PVOID)g_FT4222DevList[0].LocId, FT_OPEN_BY_LOCATION, &ftHandle);
    if (FT_OK != ftStatus)
    {
        printf("Open a FT4222 device failed!\n");
        return 0;
    }


    printf("\n\n");
    printf("Init FT4222 as I2C master\n");
    ftStatus = FT4222_I2CMaster_Init(ftHandle, 400);
    if (FT_OK != ftStatus)
    {
        printf("Init FT4222 as I2C master device failed!\n");
        return 0;
    }

    uint8_t gpioData;
    uint8_t prevData = 0xff;

    printf("\n--- Loopback Test ---\n");

    for (;;) {
        FT4222_STATUS ft4222Status = readGPIO(ftHandle, 0x26, &gpioData);
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

    printf("UnInitialize FT4222\n");
    FT4222_UnInitialize(ftHandle);

    printf("Close FT device\n");
    FT_Close(ftHandle);
    return 0;
}
