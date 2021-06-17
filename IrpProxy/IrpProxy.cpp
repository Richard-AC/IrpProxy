#include <ntddk.h>
#include "common.h"
#include "IrpProxy.h"

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

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\IrpProxy");

	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(
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


NTSTATUS IrpProxyDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_HOOK_TARGET: {
		/* Receive a driver name as an input buffer. 
		Hook the corresponding driver. */

		if (targetDriver) {
			KdPrint(("Target driver already selected!\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		if (stack->Parameters.DeviceIoControl.InputBufferLength <= 0) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		auto driverName = (PCWSTR)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (driverName == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

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
			savedMajorFunction[j] = (PDRIVER_DISPATCH) InterlockedExchangePointer(
				(PVOID*)&targetDriver->MajorFunction[j], DispatchProxy);

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
	default: {
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

/* This function will replace the target driver's handlers when hooking */
NTSTATUS DispatchProxy(PDEVICE_OBJECT DeviceObject, PIRP Irp) {

	//auto driver = DeviceObject->DriverObject;
	auto stack = IoGetCurrentIrpStackLocation(Irp);

	KdPrint(("Intercepted Irp ! \n"));

	auto status = targetDriver->MajorFunction[stack->MajorFunction](DeviceObject, Irp);

	return status;
}

