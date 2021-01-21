


#include "hv.h"

NTSTATUS
HvDriverEntry(
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath
)
{
    RegistryPath;

    DriverObject->DriverUnload = HvDriverUnload;

    HvInitializeVisor( );

    return STATUS_SUCCESS;
}

VOID
HvDriverUnload(
    __in PDRIVER_OBJECT  DriverObject
)
{
    DriverObject;

    HvTerminateVisor( );

}

