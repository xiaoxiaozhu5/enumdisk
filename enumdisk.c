/*++

Copyright (c) 1990-2000 Microsoft Corporation, All Rights Reserved

Module Name:

    Enumdisk.c

Abstract:

Please note that the objective of this sample is to demonstrate the 
techniques used. This is not a complete program to be used in 
commercial product.
 
The purpose of the sample program is to enumerates all available disk
devices and get the device property. It uses IOCTL_STORAGE_QUERY_PROPERTY 
to get the Bus and Device properties. It also opens the handle to the device 
and sends a SCSI Pass Through command.

See SDK & DDK Documentation for more information about the APIs, 
IOCTLs and data structures used.
    
Author:

    Raju Ramanathan     05/15/2000

Notes:


Revision History:

  Raju Ramanathan       05/29/2000    Completed the sample

--*/

#include <stdio.h> 
#include <stdlib.h> 
#include <stddef.h>
#include <windows.h>  
#include <initguid.h>   // Guid definition
#include <devguid.h>    // Device guids
#include <setupapi.h>   // for SetupDiXxx functions.
#include <cfgmgr32.h>   // for SetupDiXxx functions.
#include <devioctl.h>  
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <enumdisk.h>


VOID DebugPrint( USHORT DebugPrintLevel, PCHAR DebugMessage, ... )
/*++

Routine Description:

    This routine print the given string, if given debug level is <= to the
    current debug level.

Arguments:

    DebugPrintLevel - Debug level of the given message

    DebugMessage    - Message to be printed

Return Value:

  None

--*/
{

    va_list args;

    va_start(args, DebugMessage);

    if (DebugPrintLevel <= DebugLevel) {
        char buffer[128];
        (VOID) vsprintf(buffer, DebugMessage, args);
        printf( "%s", buffer );
    }

    va_end(args);
}


int __cdecl main()
/*++

Routine Description:

    This is the main function. It takes no arguments from the user.

Arguments:

    None

Return Value:

  Status

--*/
{
    HDEVINFO        hDevInfo, hIntDevInfo;
    DWORD           index;
    BOOL            status;


    hDevInfo = SetupDiGetClassDevs(
                    (LPGUID) &GUID_DEVCLASS_DISKDRIVE,
                    NULL,
                    NULL, 
                    DIGCF_PRESENT  ); // All devices present on system
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        DebugPrint( 1, "SetupDiGetClassDevs failed with error: %d\n", GetLastError()   );
        exit(1);
    }

    //
    // Open the device using device interface registered by the driver
    //

    //
    // Get the interface device information set that contains all devices of event class.
    //

    hIntDevInfo = SetupDiGetClassDevs (
                 (LPGUID)&DiskClassGuid,
                 NULL,                                   // Enumerator
                 NULL,                                   // Parent Window
                 (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE  // Only Devices present & Interface class
                 ));

    if( hDevInfo == INVALID_HANDLE_VALUE ) {
        DebugPrint( 1, "SetupDiGetClassDevs failed with error: %d\n", GetLastError() );
        exit(1);
    }

    //
    //  Enumerate all the disk devices
    //
    index = 0;

    while (TRUE) 
    {
        DebugPrint( 1, "Properties for Device %d", index+1);
        status = GetRegistryProperty( hDevInfo, index );
        if ( status == FALSE ) {
            break;
        }

        status = GetDeviceProperty( hIntDevInfo, index );
        if ( status == FALSE ) {
            break;
        }
        index++;
    }
    DebugPrint( 1, "\r ***  End of Device List  *** \n");
    SetupDiDestroyDeviceInfoList(hDevInfo);
    SetupDiDestroyDeviceInfoList(hIntDevInfo);
    return 0;
}


BOOL GetRegistryProperty( HDEVINFO DevInfo, DWORD Index )
/*++

Routine Description:

    This routine enumerates the disk devices using the Setup class interface
    GUID GUID_DEVCLASS_DISKDRIVE. Gets the Device ID from the Registry 
    property.

Arguments:

    DevInfo - Handles to the device information list

    Index   - Device member 

Return Value:

  TRUE / FALSE. This decides whether to continue or not

--*/
{

    SP_DEVINFO_DATA deviceInfoData;
    DWORD           errorCode;
    DWORD           bufferSize = 0;
    DWORD           dataType;
    LPTSTR          buffer = NULL;
    BOOL            status;

    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    status = SetupDiEnumDeviceInfo(
                DevInfo,
                Index,
                &deviceInfoData);

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_NO_MORE_ITEMS ) {
            DebugPrint( 2, "No more devices.\n");
        }
        else {
            DebugPrint( 1, "SetupDiEnumDeviceInfo failed with error: %d\n", errorCode );
        }
        return FALSE;
    }
        
    //
    // We won't know the size of the HardwareID buffer until we call
    // this function. So call it with a null to begin with, and then 
    // use the required buffer size to Alloc the necessary space.
    // Keep calling we have success or an unknown failure.
    //

    status = SetupDiGetDeviceRegistryProperty(
                DevInfo,
                &deviceInfoData,
                SPDRP_HARDWAREID,
                &dataType,
                (PBYTE)buffer,
                bufferSize,
                &bufferSize);

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode != ERROR_INSUFFICIENT_BUFFER ) {
            if ( errorCode == ERROR_INVALID_DATA ) {
                //
                // May be a Legacy Device with no HardwareID. Continue.
                //
                return TRUE;
            }
            else {
                DebugPrint( 1, "SetupDiGetDeviceInterfaceDetail failed with error: %d\n", errorCode );
                return FALSE;
            }
        }
    }

    //
    // We need to change the buffer size.
    //

    buffer = LocalAlloc(LPTR, bufferSize);
    
    status = SetupDiGetDeviceRegistryProperty(
                DevInfo,
                &deviceInfoData,
                SPDRP_HARDWAREID,
                &dataType,
                (PBYTE)buffer,
                bufferSize,
                &bufferSize);

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_INVALID_DATA ) {
            //
            // May be a Legacy Device with no HardwareID. Continue.
            //
            return TRUE;
        }
        else {
            DebugPrint( 1, "SetupDiGetDeviceInterfaceDetail failed with error: %d\n", errorCode );
            return FALSE;
        }
    }

    DebugPrint( 1, "\n\nDevice ID: %s\n",buffer );
    
    if (buffer) {
        LocalFree(buffer);
    }

    return TRUE;
}


BOOL GetDeviceProperty(HDEVINFO IntDevInfo, DWORD Index )
/*++

Routine Description:

    This routine enumerates the disk devices using the Device interface
    GUID DiskClassGuid. Gets the Adapter & Device property from the port
    driver. Then sends IOCTL through SPTI to get the device Inquiry data.

Arguments:

    IntDevInfo - Handles to the interface device information list

    Index      - Device member 

Return Value:

  TRUE / FALSE. This decides whether to continue or not

--*/
{
    SP_DEVICE_INTERFACE_DATA            interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    interfaceDetailData = NULL;
    STORAGE_PROPERTY_QUERY              query;
    PSTORAGE_ADAPTER_DESCRIPTOR         adpDesc;
    PSTORAGE_DEVICE_DESCRIPTOR          devDesc;
    SCSI_PASS_THROUGH_WITH_BUFFERS      sptwb;
    HANDLE                              hDevice;
    BOOL                                status;
    PUCHAR                              p;
    UCHAR                               outBuf[512];
    ULONG                               length = 0,
                                        returned = 0,
                                        returnedLength;
    DWORD                               interfaceDetailDataSize,
                                        reqSize,
                                        errorCode, 
                                        i;


    interfaceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

    status = SetupDiEnumDeviceInterfaces ( 
                IntDevInfo,             // Interface Device Info handle
                0,                      // Device Info data
                (LPGUID)&DiskClassGuid, // Interface registered by driver
                Index,                  // Member
                &interfaceData          // Device Interface Data
                );

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_NO_MORE_ITEMS ) {
            DebugPrint( 2, "No more interfaces\n" );
        }
        else {
            DebugPrint( 1, "SetupDiEnumDeviceInterfaces failed with error: %d\n", errorCode  );
        }
        return FALSE;
    }
        
    //
    // Find out required buffer size, so pass NULL 
    //

    status = SetupDiGetDeviceInterfaceDetail (
                IntDevInfo,         // Interface Device info handle
                &interfaceData,     // Interface data for the event class
                NULL,               // Checking for buffer size
                0,                  // Checking for buffer size
                &reqSize,           // Buffer size required to get the detail data
                NULL                // Checking for buffer size
                );

    //
    // This call returns ERROR_INSUFFICIENT_BUFFER with reqSize 
    // set to the required buffer size. Ignore the above error and
    // pass a bigger buffer to get the detail data
    //

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode != ERROR_INSUFFICIENT_BUFFER ) {
            DebugPrint( 1, "SetupDiGetDeviceInterfaceDetail failed with error: %d\n", errorCode   );
            return FALSE;
        }
    }

    //
    // Allocate memory to get the interface detail data
    // This contains the devicepath we need to open the device
    //

    interfaceDetailDataSize = reqSize;
    interfaceDetailData = malloc (interfaceDetailDataSize);
    if ( interfaceDetailData == NULL ) {
        DebugPrint( 1, "Unable to allocate memory to get the interface detail data.\n" );
        return FALSE;
    }
    interfaceDetailData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    status = SetupDiGetDeviceInterfaceDetail (
                  IntDevInfo,               // Interface Device info handle
                  &interfaceData,           // Interface data for the event class
                  interfaceDetailData,      // Interface detail data
                  interfaceDetailDataSize,  // Interface detail data size
                  &reqSize,                 // Buffer size required to get the detail data
                  NULL);                    // Interface device info

    if ( status == FALSE ) {
        DebugPrint( 1, "Error in SetupDiGetDeviceInterfaceDetail failed with error: %d\n", GetLastError() );
        return FALSE;
    }

    //
    // Now we have the device path. Open the device interface
    // to send Pass Through command

    DebugPrint( 2, "Interface: %s\n", interfaceDetailData->DevicePath);

    hDevice = CreateFile(
                interfaceDetailData->DevicePath,    // device interface name
                GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                NULL,                               // lpSecurityAttributes
                OPEN_EXISTING,                      // dwCreationDistribution
                0,                                  // dwFlagsAndAttributes
                NULL                                // hTemplateFile
                );
                
    //
    // We have the handle to talk to the device. 
    // So we can release the interfaceDetailData buffer
    //

    free (interfaceDetailData);

    if (hDevice == INVALID_HANDLE_VALUE) {
        DebugPrint( 1, "CreateFile failed with error: %d\n", GetLastError() );
        return TRUE;
    }

    query.PropertyId = StorageAdapterProperty;
    query.QueryType = PropertyStandardQuery;

    status = DeviceIoControl(
                        hDevice,                
                        IOCTL_STORAGE_QUERY_PROPERTY,
                        &query,
                        sizeof( STORAGE_PROPERTY_QUERY ),
                        &outBuf,                   
                        512,                      
                        &returnedLength,      
                        NULL                    
                        );
    if ( !status ) {
        DebugPrint( 1, "IOCTL failed with error code%d.\n\n", GetLastError() );
    }
    else {
        adpDesc = (PSTORAGE_ADAPTER_DESCRIPTOR) outBuf;
        DebugPrint( 1, "\nAdapter Properties\n");
        DebugPrint( 1, "------------------\n");
        DebugPrint( 1, "Bus Type       : %s\n", BusType[adpDesc->BusType]);
        DebugPrint( 1, "Max. Tr. Length: 0x%x\n", adpDesc->MaximumTransferLength );
        DebugPrint( 1, "Max. Phy. Pages: 0x%x\n", adpDesc->MaximumPhysicalPages );
        DebugPrint( 1, "Alignment Mask : 0x%x\n", adpDesc->AlignmentMask );

        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        status = DeviceIoControl(
                            hDevice,                
                            IOCTL_STORAGE_QUERY_PROPERTY,
                            &query,
                            sizeof( STORAGE_PROPERTY_QUERY ),
                            &outBuf,                   
                            512,                      
                            &returnedLength,
                            NULL                    
                            );
        if ( !status ) {
            DebugPrint( 1, "IOCTL failed with error code%d.\n\n", GetLastError() );
        }
        else {
            DebugPrint( 1, "\nDevice Properties\n");
            DebugPrint( 1, "-----------------\n");
            devDesc = (PSTORAGE_DEVICE_DESCRIPTOR) outBuf;
            //
            // Our device table can handle only 16 devices.
            //
            DebugPrint( 1, "Device Type     : %s (0x%X)\n", 
                DeviceType[devDesc->DeviceType > 0x0F? 0x0F: devDesc->DeviceType ], devDesc->DeviceType);
            if ( devDesc->DeviceTypeModifier ) {
                DebugPrint( 1, "Device Modifier : 0x%x\n", devDesc->DeviceTypeModifier);
            }

            DebugPrint( 1, "Removable Media : %s\n", devDesc->RemovableMedia ? "Yes" : "No" );
            p = (PUCHAR) outBuf; 

            if ( devDesc->VendorIdOffset && p[devDesc->VendorIdOffset] ) {
                DebugPrint( 1, "Vendor ID       : " );
                for ( i = devDesc->VendorIdOffset; p[i] != (UCHAR) NULL && i < returnedLength; i++ ) {
                    DebugPrint( 1, "%c", p[i] );
                }
                DebugPrint( 1, "\n");
            }
            if ( devDesc->ProductIdOffset && p[devDesc->ProductIdOffset] ) {
                DebugPrint( 1, "Product ID      : " );
                for ( i = devDesc->ProductIdOffset; p[i] != (UCHAR) NULL && i < returnedLength; i++ ) {
                    DebugPrint( 1, "%c", p[i] );
                }
                DebugPrint( 1, "\n");
            }

            if ( devDesc->ProductRevisionOffset && p[devDesc->ProductRevisionOffset] ) {
                DebugPrint( 1, "Product Revision: " );
                for ( i = devDesc->ProductRevisionOffset; p[i] != (UCHAR) NULL && i < returnedLength; i++ ) {
                    DebugPrint( 1, "%c", p[i] );
                }
                DebugPrint( 1, "\n");
            }

            if ( devDesc->SerialNumberOffset && p[devDesc->SerialNumberOffset] ) {
                DebugPrint( 1, "Serial Number   : " );
                for ( i = devDesc->SerialNumberOffset; p[i] != (UCHAR) NULL && i < returnedLength; i++ ) {
                    DebugPrint( 1, "%c", p[i] );
                }
                DebugPrint( 1, "\n");
            }
        }
    }


    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.Spt.PathId = 0;
    sptwb.Spt.TargetId = 1;
    sptwb.Spt.Lun = 0;
    sptwb.Spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.Spt.SenseInfoLength = 24;
    sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.Spt.DataTransferLength = 192;
    sptwb.Spt.TimeOutValue = 2;
    sptwb.Spt.DataBufferOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf);
    sptwb.Spt.SenseInfoOffset = 
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,SenseBuf);
    sptwb.Spt.Cdb[0] = SCSIOP_INQUIRY;
    sptwb.Spt.Cdb[4] = 192;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf) +
       sptwb.Spt.DataTransferLength;

    status = DeviceIoControl(hDevice,
                             IOCTL_SCSI_PASS_THROUGH,
                             &sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &sptwb,
                             length,
                             &returned,
                             FALSE);

    PrintStatusResults(status, returned, &sptwb);

    //
    // Close handle the driver
    //
    if ( !CloseHandle(hDevice) )     {
        DebugPrint( 2, "Failed to close device.\n");
    }

    return TRUE;
}



VOID
PrintError(ULONG ErrorCode)
/*++

Routine Description:

    Prints formated error message

Arguments:

    ErrorCode   - Error code to print


Return Value:
    
      None
--*/
{
    UCHAR errorBuffer[80];
    ULONG count;

    count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    ErrorCode,
                    0,
                    errorBuffer,
                    sizeof(errorBuffer),
                    NULL
                    );

    if (count != 0) {
        DebugPrint( 1, "%s\n", errorBuffer);
    } else {
        DebugPrint( 1, "Format message failed.  Error: %d\n", GetLastError());
    }
}

VOID
PrintDataBuffer(PUCHAR DataBuffer, ULONG BufferLength)
/*++

Routine Description:

    Prints the formated data buffer

Arguments:

    DataBuffer      - Buffer to be printed

    BufferLength    - Length of the buffer


Return Value:
    
      None
--*/
{
    ULONG cnt;

    DebugPrint( 3, "      00  01  02  03  04  05  06  07   08  09  0A  0B  0C  0D  0E  0F\n");
    DebugPrint( 3, "      ---------------------------------------------------------------\n");
    for (cnt = 0; cnt < BufferLength; cnt++) {
        if ((cnt) % 16 == 0) {
            DebugPrint( 3, " %03X  ",cnt);
            }
        DebugPrint( 3, "%02X  ", DataBuffer[cnt]);
        if ((cnt+1) % 8 == 0) {
            DebugPrint( 3, " ");
        }
        if ((cnt+1) % 16 == 0) {
            DebugPrint( 3, "\n");
        }
    }
    DebugPrint( 3, "\n\n");
}


VOID
PrintStatusResults( BOOL Status, DWORD Returned, PSCSI_PASS_THROUGH_WITH_BUFFERS Psptwb )
/*++

Routine Description:

    Prints the SCSI Inquiry data from the device

Arguments:

    Status      - Status of the DeviceIOControl

    Returned    - Number of bytes returned

    Psptwb      - SCSI pass through structure

Return Value:
    
      None
--*/
{
    ULONG   errorCode;
    USHORT  i, devType;

    DebugPrint( 1, "\nInquiry Data from Pass Through\n");
    DebugPrint( 1, "------------------------------\n");

    if (!Status ) {
        DebugPrint( 1, "Error: %d ", errorCode = GetLastError() );
        PrintError(errorCode);
        return;
    }
    if (Psptwb->Spt.ScsiStatus) {
        PrintSenseInfo(Psptwb);
        return;
    }
    else {
        devType = Psptwb->DataBuf[0] & 0x1f ;

        //
        // Our Device Table can handle only 16 devices.
        //
        DebugPrint( 1, "Device Type: %s (0x%X)\n", DeviceType[devType > 0x0F? 0x0F: devType], devType );

        DebugPrint( 1, "Vendor ID  : ");
        for (i = 8; i <= 15; i++) {
            DebugPrint( 1, "%c", Psptwb->DataBuf[i] );
        }
        DebugPrint( 1, "\nProduct ID : ");
        for (i = 16; i <= 31; i++) {
            DebugPrint( 1, "%c", Psptwb->DataBuf[i] );
        }
        DebugPrint( 1, "\nProduct Rev: ");
        for (i = 32; i <= 35; i++) {
            DebugPrint( 1, "%c", Psptwb->DataBuf[i] );
        }
        DebugPrint( 1, "\nVendor Str : ");
        for (i = 36; i <= 55; i++) {
            DebugPrint( 1, "%c", Psptwb->DataBuf[i] );
        }
        DebugPrint( 1, "\n\n");
        DebugPrint( 3, "Scsi status: %02Xh, Bytes returned: %Xh, ",
            Psptwb->Spt.ScsiStatus, Returned);
        DebugPrint( 3, "Data buffer length: %Xh\n\n\n",
            Psptwb->Spt.DataTransferLength);
        PrintDataBuffer( Psptwb->DataBuf, 192);
        DebugPrint( 1, "\n\n");
    }
}

VOID
PrintSenseInfo(PSCSI_PASS_THROUGH_WITH_BUFFERS Psptwb)
/*++

Routine Description:

    Prints the SCSI status and Sense Info from the device

Arguments:

    Psptwb      - Pass Through buffer that contains the Sense info


Return Value:
    
      None
--*/
{
    int i;

    DebugPrint( 1, "Scsi status: %02Xh\n\n", Psptwb->Spt.ScsiStatus);
    if (Psptwb->Spt.SenseInfoLength == 0) {
        return;
    }
    DebugPrint( 3, "Sense Info -- consult SCSI spec for details\n");
    DebugPrint( 3, "-------------------------------------------------------------\n");
    for (i=0; i < Psptwb->Spt.SenseInfoLength; i++) {
        DebugPrint( 3, "%02X ", Psptwb->SenseBuf[i]);
    }
    DebugPrint( 1, "\n\n");
}
