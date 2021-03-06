#pragma once
#include <ntddk.h>
#include "IrpHandler.h"
#include "CyclicBuffer.h"
#include "SimpleTable.h"
#include "common.h"

const ULONG DRIVER_TAG = 'YXRP';
const int MaxDataSize = 1 << 13;

extern "C"
NTSYSAPI
NTSTATUS NTAPI ObReferenceObjectByName(
	_In_ PUNICODE_STRING ObjectPath,
	_In_ ULONG Attributes,
	_In_opt_ PACCESS_STATE PassedAccessState,
	_In_opt_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_TYPE ObjectType,
	_In_ KPROCESSOR_MODE AccessMode,
	_Inout_opt_ PVOID ParseContext,
	_Out_ PVOID *Object);

extern "C" POBJECT_TYPE* IoDriverObjectType;
