#include "IrpProxy.h"

CyclicBuffer<SpinLock>* DataBuffer;
PDRIVER_OBJECT targetDriver;
PDRIVER_DISPATCH savedMajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

void IrpProxyUnload(PDRIVER_OBJECT);
NTSTATUS IrpProxyCreateClose(PDEVICE_OBJECT, PIRP);
NTSTATUS IrpProxyDeviceControl(PDEVICE_OBJECT, PIRP);
NTSTATUS DispatchProxy(PDEVICE_OBJECT, PIRP);

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = IrpProxyUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpProxyCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpProxyCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpProxyDeviceControl;

	DataBuffer = new (NonPagedPool) CyclicBuffer<SpinLock>;
	if (DataBuffer == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
		
	}

	NTSTATUS status = DataBuffer->Init(1 << 20, NonPagedPool, DRIVER_TAG);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\IrpProxy");

	PDEVICE_OBJECT DeviceObject;
	status = IoCreateDevice(
		DriverObject,
		0,
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&DeviceObject
	);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status;
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\IrpProxy");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if(!NT_SUCCESS(status)){
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	return STATUS_SUCCESS;
}

void IrpProxyUnload(PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\IrpProxy");
	IoDeleteSymbolicLink(&symLink);

	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS IrpProxyCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR Information) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = Information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS IrpProxyDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;
	auto inputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	auto outputLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG_PTR information = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_HOOK_TARGET: {
		/* Receive a driver name as an input buffer. 
		Hook the corresponding driver. */

		if (targetDriver) {
			KdPrint(("Target driver already selected!\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		if (inputLen <= 0) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		auto driverName = (PCWSTR)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (driverName == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		__try {
			KdPrint(("Attempting to hook : %ws\n", driverName));

			UNICODE_STRING name;
			RtlInitUnicodeString(&name, driverName);
			status = ObReferenceObjectByName(&name, OBJ_CASE_INSENSITIVE,
				nullptr, 0, *IoDriverObjectType, KernelMode,
				nullptr, (PVOID*)&targetDriver);

			if (!NT_SUCCESS(status)) {
				KdPrint(("Failed to obtain driver reference from name : %ws.\n", driverName));
				break;
			}

			for (int j = 0; j <= IRP_MJ_MAXIMUM_FUNCTION; j++) {
				savedMajorFunction[j] = (PDRIVER_DISPATCH)InterlockedExchangePointer(
					(PVOID*)&targetDriver->MajorFunction[j], DispatchProxy);

			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			status = STATUS_ACCESS_VIOLATION;
		}


		break;
	}

	case IOCTL_UNHOOK_TARGET: {
		if (targetDriver == nullptr) {
			KdPrint(("No hooked target to unhook!\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		KdPrint(("Attempting to unhook driver.\n"));

		for (int j = 0; j <= IRP_MJ_MAXIMUM_FUNCTION; j++) {
			savedMajorFunction[j] = (PDRIVER_DISPATCH) InterlockedExchangePointer(
				(PVOID*)&targetDriver->MajorFunction[j], savedMajorFunction[j]);

		}

		ObDereferenceObject(targetDriver);

		break;
	}

	case IOCTL_GET_DATA: {
		if (outputLen < sizeof(CommonInfoHeader)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		auto buffer = static_cast<PUCHAR>(MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority));
		if (buffer == nullptr) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		information = DataBuffer->Read(buffer, outputLen);
		break;
		}
	default: {
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}


	CompleteRequest(Irp, status, information);

	return status;
}

/* This function will replace the target driver's handlers when hooking */
NTSTATUS DispatchProxy(PDEVICE_OBJECT DeviceObject, PIRP Irp) {

	//auto driver = DeviceObject->DriverObject;
	auto stack = IoGetCurrentIrpStackLocation(Irp);

	KdPrint(("Intercepted Irp !\n"));

	switch(stack->MajorFunction){
		case IRP_MJ_CREATE: 
			KdPrint(("Intercepted IRP_MJ_CREATE!\n"));
			break;
		
		case IRP_MJ_CLOSE: 
			KdPrint(("Intercepted IRP_MJ_CLOSE!\n"));
			break;

		case IRP_MJ_READ:
			KdPrint(("Intercepted IRP_MJ_READ!\n"));
			break;

		case IRP_MJ_WRITE:
			KdPrint(("Intercepted IRP_MJ_WRITE!\n"));
			break;

		case IRP_MJ_DEVICE_CONTROL: 
			KdPrint(("Intercepted IRP_MJ_DEVICE_CONTROL!\n"));
			break;

		default: 
			KdPrint(("Intercepted unknown IRP : %d", stack->MajorFunction));
	}

	HandleIrp(DeviceObject, Irp, stack);

	auto status = savedMajorFunction[stack->MajorFunction](DeviceObject, Irp);

	return status;
}

