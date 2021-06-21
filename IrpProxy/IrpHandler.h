#include "common.h"
#include "IrpProxy.h"

void* __cdecl operator new(size_t size, POOL_TYPE type, ULONG tag = 0);


NTSTATUS HandleIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack);

NTSTATUS GetDataFromIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack, UCHAR MajorFunction, PVOID buffer, ULONG size, bool output = false);
