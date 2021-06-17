#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "..\IrpProxy\common.h"

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
		
		const size_t WCHARBUF = 100;
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
			printf("Echo succeeded!\n");
		}
		else {
			Error("Echo failed!\n");
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
			argv[2], strlen(argv[2]),
			nullptr, 0,
			&returned, nullptr
		);

		if (success) {
			printf("Echo succeeded!\n");
		}
		else {
			Error("Echo failed!\n");
		}
		
		CloseHandle(hDevice);
	}
	else {
		printf("Invalid argument. Available arguments : /hook and /unhook\n");
		printf("Usage : IrpProxyClient /action <DriverName>\n");
		return 1;
	}


	return 0;
}