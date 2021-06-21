#include "IrpProxy.h"

NTSTATUS GetDataFromIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack, UCHAR MajorFunction, PVOID buffer, ULONG size, bool output) {
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
				if (stack->Parameters.DeviceIoControl.Type3InputBuffer < (PVOID)(1 << 16)) {
					::memcpy(buffer, stack->Parameters.DeviceIoControl.Type3InputBuffer, size);
				}
				else {
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

NTSTATUS HandleIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp) {

	NTSTATUS status = STATUS_UNSUCCESSFUL; 
	return STATUS_SUCCESS;
}
