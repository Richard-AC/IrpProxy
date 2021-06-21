#include "common.h"

void* __cdecl operator new(size_t size, POOL_TYPE type, ULONG tag = 0);

#pragma pack(push, 1)

enum class DataItemType : short {
    IrpArrived,
    IrpCompleted,
};

struct CommonInfoHeader {
    USHORT Size;
    DataItemType Type;
    long long Time;
	ULONG ProcessId;
	ULONG ThreadId;
};

struct IrpArrivedInfo : CommonInfoHeader {
    PVOID DeviceObject;
    PVOID DriverObject;
    PVOID Irp;
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Irql;
    UCHAR _padding;
    ULONG DataSize;
    union {
        struct {
            ULONG IoControlCode;
            ULONG InputBufferLength;
            ULONG OutputBufferLength;
        } DeviceIoControl;
        struct {
            USHORT FileAttributes;
            USHORT ShareAccess;
        } Create;
        struct {
            ULONG Length;
            long long Offset;
        } Read;
        struct {
            ULONG Length;
            long long Offset;
        } Write;
    };
};

struct IrpCompletedInfo : CommonInfoHeader {
    PVOID Irp;
    long Status;
    ULONG_PTR Information;
	ULONG DataSize;
};

#pragma pack(pop)

NTSTATUS HandleIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack);

NTSTATUS GetDataFromIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack, UCHAR MajorFunction, PVOID buffer, ULONG size, bool output = false);
