#include <ntddk.h>

void IrpProxyUnload(PDRIVER_OBJECT);
NTSTATUS IrpProxyCreateClose(PDEVICE_OBJECT, PIRP);
NTSTATUS IrpProxyDeviceControl(PDEVICE_OBJECT, PIRP);

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = IrpProxyUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpProxyCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpProxyCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpProxyDeviceControl;
}

