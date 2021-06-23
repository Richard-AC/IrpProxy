#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "..\IrpProxy\common.h"

#define FILENAME "recorded_irps.dat"

void DisplayIrps(IrpArrivedInfo* irpArrived);

int Error(const char* message) {
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}

int main(int argc, char* argv[]) {

	if (argc == 3 && strcmp(argv[1], "/hook") == 0) {

		HANDLE hDevice = CreateFile(L"\\\\.\\IrpProxy", GENERIC_WRITE, 
			FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

		if (hDevice == INVALID_HANDLE_VALUE) {
			return Error("Failed to open device");
		}
		
		const size_t WCHARBUF = MAX_PATH;
		wchar_t  driverNameWide[WCHARBUF];
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[2], -1, driverNameWide, WCHARBUF);	

		printf("Trying to hook : %ws\n", driverNameWide);


		DWORD returned; 
		BOOL success = DeviceIoControl(
			hDevice,
			IOCTL_HOOK_TARGET,
			driverNameWide, wcslen(driverNameWide)*2+2,
			nullptr, 0,
			&returned, nullptr
		);

		if (success) {
			printf("Hooking succeeded!\n");
		}
		else {
			Error("Hooking failed!\n");
		}
		
		CloseHandle(hDevice);
	}
	else if (argc == 2 && strcmp(argv[1], "/unhook") == 0){
	
		HANDLE hDevice = CreateFile(L"\\\\.\\IrpProxy", GENERIC_WRITE, 
			FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

		if (hDevice == INVALID_HANDLE_VALUE) {
			return Error("Failed to open device");
		}
		
		DWORD returned; 
		BOOL success = DeviceIoControl(
			hDevice,
			IOCTL_UNHOOK_TARGET,
			nullptr, 0,
			nullptr, 0,
			&returned, nullptr
		);

		if (success) {
			printf("Unhooking succeeded!\n");
		}
		else {
			Error("Unhooking failed!\n");
		}
		
		CloseHandle(hDevice);
	}

	else if (argc == 2 && strcmp(argv[1], "/getData") == 0) {
		HANDLE hDevice = CreateFile(L"\\\\.\\IrpProxy", GENERIC_WRITE, 
			FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

		if (hDevice == INVALID_HANDLE_VALUE) {
			return Error("Failed to open device");
		}

		UINT32 size = 0x2000;

		IrpArrivedInfo *irpArrived = (IrpArrivedInfo*) malloc(size);
		if (!irpArrived) {
			printf("malloc failed");
			return 1;
		}
		//IrpArrivedInfo irpArrived;
		memset(irpArrived, 0, size);
		
		DWORD returned; 
		BOOL success = DeviceIoControl(
			hDevice,
			IOCTL_GET_DATA,
			nullptr, 0,
			irpArrived, size,
			&returned, nullptr
		);
		if (success) {
			printf("Retreived data!\n\n");

			if (irpArrived->Size) {
				DisplayIrps(irpArrived);
			}

		}
		else {
			Error("Failed to get data!\n");
		}

		free(irpArrived);
		
		CloseHandle(hDevice);

	}
	else {
		printf("Invalid argument. Available arguments : /hook and /unhook\n");
		printf("Usage : IrpProxyClient /action <DriverName>\n");
		return 1;
	}


	return 0;
}

void DisplayIrps(IrpArrivedInfo* irpArrived) {
	HANDLE hFile;
	bool bErrorFlag = false;
	DWORD dwBytesWritten = 0;
	IrpArrivedInfo* current_irp = irpArrived; 

	hFile = CreateFileA(FILENAME, 
				   FILE_APPEND_DATA,   
				   0,               
				   NULL,           
				   OPEN_ALWAYS,
				   FILE_ATTRIBUTE_NORMAL,
				   NULL);               

	while (current_irp->Size) {
		printf("\n[*] Size : %d\n", current_irp->Size);
		printf("MajorFunction : %d\n", current_irp->MajorFunction);
		printf("DataSize: %d\n", current_irp->DataSize);

		if (current_irp->DataSize) {
			printf("Data:\n");
			for (int i = 0; i < current_irp->DataSize; i++) {
				printf("%02x", *((char*)current_irp + sizeof(IrpArrivedInfo) + i));
			}
			printf("\n");
		}

		bErrorFlag = WriteFile( 
				hFile,           
				current_irp,     
				current_irp->Size,
				&dwBytesWritten,
				NULL);        

		if (dwBytesWritten != current_irp->Size) {
			Error("Error while writing to file\n");
		}

		current_irp = (IrpArrivedInfo*) ((UINT64) current_irp + current_irp->Size);
	}

	CloseHandle(hFile);
}