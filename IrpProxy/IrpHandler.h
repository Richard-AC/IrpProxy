#include <ntddk.h>
#include "common.h"

typedef struct
{
    UINT32 Pid;
    UINT32 Tid;
    UINT32 IoctlCode;
    UINT32 Type;
    WCHAR DriverName[MAX_PATH];
    WCHAR DeviceName[MAX_PATH];
    PVOID InputBuffer;
    PVOID OutputBuffer;
    ULONG InputBufferLen;
    ULONG OutputBufferLen;
}
IRP_INFO, *PIRP_INFO;

NTSTATUS HandleIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS GetDataFromIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack, UCHAR MajorFunction, PVOID buffer, ULONG size, bool output);
