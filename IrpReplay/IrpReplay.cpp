#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "..\IrpProxy\common.h"

#define IRP_MJ_CREATE 0x00
#define IRP_MJ_CLOSE 0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f

void ReplayIrps(IrpArrivedInfo*, HANDLE);

int Error(const char* message) {
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		printf("Usage : IrpReplay <FileName.dat> <DeviceName>\n");
		return 1;
	}

	wchar_t DevicePath[MAX_PATH] = L"\\\\.\\";
	wchar_t  DeviceName[MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[2], -1, DeviceName, MAX_PATH);	
	wcscat_s(DevicePath, MAX_PATH, DeviceName);

	HANDLE hDevice = CreateFile(DevicePath, GENERIC_WRITE, 
		FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE) {
		return Error("Failed to open device");
	}

	HANDLE hFile;
	bool bErrorFlag = false;
	DWORD dwBytesRead = 0;

	hFile = CreateFileA(argv[1], 
				   GENERIC_READ,   
				   0,               
				   NULL,           
				   OPEN_EXISTING,
				   FILE_ATTRIBUTE_NORMAL,
				   NULL);               

	if (hFile == INVALID_HANDLE_VALUE) {
		CloseHandle(hDevice);
		return Error("Failed to open file");
	}

	IrpArrivedInfo* current_irp = (IrpArrivedInfo*) malloc(0x1000);
	IrpArrivedInfo* saved_irps = (IrpArrivedInfo*) malloc(0x1000);
	if (current_irp == nullptr) {

		return Error("Failed to allocate buffer.\n");
		CloseHandle(hFile);
		CloseHandle(hDevice);
	}

	memset(current_irp, 0, 0x1000);
	memset(saved_irps, 0, 0x1000);
	bErrorFlag = ReadFile(hFile,
		current_irp,
		0x1000,
		&dwBytesRead,
		NULL
	);

	if (!bErrorFlag) {
		CloseHandle(hFile);
		CloseHandle(hDevice);
		return Error("Failed to read from file.\n");
	}

	memcpy(saved_irps, current_irp, 0x1000);

	while (true) {
		__try {
			printf("bla\n");
			memcpy(current_irp, saved_irps, 0x1000);

			/* Mutate current_irp here */
			int selected_bytes[0xa];
			for (int i = 0; i < 0xa; i++) {
				selected_bytes[i] = rand() % 0x1000;
			}

			for (int i = 0; i < 0xa; i++) {
				*((char*)current_irp + selected_bytes[i]) = rand() % 0xff;
				//printf("Index : %d is now %d", selected_bytes[i], *(char*)current_irp + selected_bytes[i]);
			}

			/***/

			ReplayIrps(current_irp, hDevice);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			printf("walafzef\n");
		}
	}
	CloseHandle(hFile);
	CloseHandle(hDevice);

	printf("WTF\n");

	return 0;
}

void ReplayIrps(IrpArrivedInfo* current_irp, HANDLE hDevice){
	while (current_irp->Size) {
		switch (current_irp->MajorFunction) {
		case IRP_MJ_CREATE:
		case IRP_MJ_CLOSE:
			break;

		case IRP_MJ_READ: {
			DWORD read_buffer_len = current_irp->Read.Length;
			void* read_buffer = nullptr;
			if (read_buffer_len != 0) {
				read_buffer = malloc(read_buffer_len);
			}
			ReadFile(hDevice, read_buffer, read_buffer_len, nullptr, NULL);
			free(read_buffer);
			break;
		}

		case IRP_MJ_WRITE: {
			DWORD write_buffer_len = current_irp->Write.Length;
			void* write_buffer = (void*)((UINT64)current_irp + sizeof(IrpArrivedInfo));
			WriteFile(hDevice, write_buffer, write_buffer_len, nullptr, NULL);
			break; 
		}

		case IRP_MJ_DEVICE_CONTROL: {
			DWORD output_buffer_len = current_irp->DeviceIoControl.OutputBufferLength;
			void* output_buffer = nullptr;
			if (output_buffer_len != 0) {
				output_buffer = malloc(current_irp->DeviceIoControl.OutputBufferLength);
			}
			BOOL success = DeviceIoControl(
				hDevice,
				current_irp->DeviceIoControl.IoControlCode,
				(void*)((UINT64)current_irp + sizeof(IrpArrivedInfo)),
				current_irp->DeviceIoControl.InputBufferLength,
				output_buffer, output_buffer_len,
				nullptr, nullptr
			);
			free(output_buffer);
		}

		}

		current_irp = (IrpArrivedInfo*) ((UINT64) current_irp + current_irp->Size);
	}
	
}


