#include "IrpProxy.h"

extern CyclicBuffer<SpinLock>* DataBuffer;

NTSTATUS GetDataFromIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack, UCHAR MajorFunction, PVOID buffer, ULONG size, bool output) {
	KdPrint(("Attempting to get data from IRP: %p", Irp));
	__try {
		switch (MajorFunction) {
		case IRP_MJ_WRITE: 
		case IRP_MJ_READ:
			if (Irp->MdlAddress) {
				auto p = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
				if (p) {
					::memcpy(buffer, p, size);
					return STATUS_SUCCESS;
				}
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			
			if (DeviceObject->Flags & DO_BUFFERED_IO) {
				if (!Irp->AssociatedIrp.SystemBuffer) {
					return STATUS_INVALID_PARAMETER;
				}
				::memcpy(buffer, Irp->AssociatedIrp.SystemBuffer, size);
				return STATUS_SUCCESS;
			}
			if (!Irp->UserBuffer) {
				return STATUS_INVALID_PARAMETER;
			}
			::memcpy(buffer, Irp->UserBuffer, size);
			return STATUS_SUCCESS;

		case IRP_MJ_DEVICE_CONTROL:
		case IRP_MJ_INTERNAL_DEVICE_CONTROL:
			auto controlCode = stack->Parameters.DeviceIoControl.IoControlCode;
			if (METHOD_FROM_CTL_CODE(controlCode) == METHOD_NEITHER) {
				KdPrint(("IRP: %p Uses method neither", Irp));
				if (stack->Parameters.DeviceIoControl.Type3InputBuffer) {
					__try {
						::memcpy(buffer, stack->Parameters.DeviceIoControl.Type3InputBuffer, size);
					}
					__except(EXCEPTION_EXECUTE_HANDLER){
						KdPrint(("Failed to retreive data from Irp\n"));
						return STATUS_UNSUCCESSFUL;
					}
				}
				else {
					KdPrint(("Failed to retreive data from Irp\n"));
					return STATUS_UNSUCCESSFUL;
				}
			}
			else {
				if (!output || METHOD_FROM_CTL_CODE(controlCode) == METHOD_BUFFERED) {
					if (!Irp->AssociatedIrp.SystemBuffer) {
						return STATUS_INVALID_PARAMETER;
					}
					::memcpy(buffer, Irp->AssociatedIrp.SystemBuffer, size);
				}
				else {
					if (!Irp->MdlAddress) {
						return STATUS_INVALID_PARAMETER;
					}
					auto data = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
					if (data) {
						::memcpy(buffer, data, size);
					}
					else {
						return STATUS_UNSUCCESSFUL;
					}
				}
			}
			return STATUS_SUCCESS;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS HandleIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack) {

	auto driver = DeviceObject->DriverObject;
	KdPrint(("Handling IRP : %p for driver: %p\n", Irp, driver));

	IrpArrivedInfo* info = nullptr;
	info = static_cast<IrpArrivedInfo*>(ExAllocatePoolWithTag(NonPagedPool, MaxDataSize + sizeof(IrpArrivedInfo), DRIVER_TAG));
			if (info) {
				info->Type = DataItemType::IrpArrived;
				KeQuerySystemTime((PLARGE_INTEGER)&info->Time);
				info->Size = sizeof(IrpArrivedInfo);
				info->DeviceObject = DeviceObject;
				info->Irp = Irp;
				info->DriverObject = driver;
				info->MajorFunction = stack->MajorFunction;
				info->MinorFunction = stack->MinorFunction;
				info->ProcessId = HandleToULong(PsGetCurrentProcessId());
				info->ThreadId = HandleToULong(PsGetCurrentThreadId());
				info->Irql = KeGetCurrentIrql();
				info->DataSize = 0;

				switch (info->MajorFunction) {
					case IRP_MJ_WRITE:
						info->Write.Length = stack->Parameters.Write.Length;
						info->Write.Offset = stack->Parameters.Write.ByteOffset.QuadPart;
						
						if (info->Write.Length > 0) {
							auto dataSize = min(MaxDataSize, info->Write.Length);
							if (NT_SUCCESS(GetDataFromIrp(DeviceObject, Irp, stack, info->MajorFunction, (PUCHAR)info + sizeof(IrpArrivedInfo), dataSize))) {
								info->DataSize = dataSize;
								info->Size += (USHORT)dataSize;
							}
						}
						break;

					case IRP_MJ_DEVICE_CONTROL:
					case IRP_MJ_INTERNAL_DEVICE_CONTROL:
						info->DeviceIoControl.IoControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
						info->DeviceIoControl.InputBufferLength = stack->Parameters.DeviceIoControl.InputBufferLength;
						info->DeviceIoControl.OutputBufferLength = stack->Parameters.DeviceIoControl.OutputBufferLength;
						if (info->DeviceIoControl.InputBufferLength > 0) {
							auto dataSize = min(MaxDataSize, info->DeviceIoControl.InputBufferLength);
							if (NT_SUCCESS(GetDataFromIrp(DeviceObject, Irp, stack, info->MajorFunction, (PUCHAR)info + sizeof(IrpArrivedInfo), dataSize))) {
								info->DataSize = dataSize;
								info->Size += (USHORT)dataSize;
								KdPrint(("Intercepted data : %s\n", (PUCHAR)info + sizeof(IrpArrivedInfo)));
							}

						}
						break;
				}

				if (!DataBuffer) {
					KdPrint(("Cyclic buffer not found\n"));
					return STATUS_UNSUCCESSFUL;
				}
				DataBuffer->Write(info, info->Size);

			}

	return STATUS_SUCCESS;
}
