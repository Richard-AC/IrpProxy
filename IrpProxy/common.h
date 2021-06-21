#pragma once

#define IRPPROXY_DEVICE 0x8000
#define IOCTL_HOOK_TARGET CTL_CODE(IRPPROXY_DEVICE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_UNHOOK_TARGET CTL_CODE(IRPPROXY_DEVICE, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_GET_DATA CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define MAX_PATH 260

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
