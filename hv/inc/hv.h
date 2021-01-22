


#pragma once

#pragma pack( 1 )

#pragma warning(disable:4200)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#pragma warning(disable:4103) // windows is really that shit.

#pragma pack( 8 )
#include <ntifs.h>
#pragma pack( 1 )

#include <intrin.h>

#define HV_POOL_TAG                         '  VH'

#define HvBitRound( x, y )                   ( ( ( x ) + ( y ) - 1 ) & ~( ( y ) - 1 ) )

#undef  RtlZeroMemory
#define RtlZeroMemory( dst, size )           memset( ( void* )( dst ), 0, ( size_t )( size ) )

#undef  C_ASSERT
#define C_ASSERT( e )                       static_assert( e, #e )

#define EXTERN                              extern

typedef NTSTATUS HVSTATUS;

#define HVSTATUS_SUCCESS                    ( ( HVSTATUS )( ( FACILITY_HYPERVISOR << 16 ) | ( STATUS_SEVERITY_SUCCESS << 30 ) | ( 1 << 29 ) ) )
#define HVSTATUS_UNSUCCESSFUL               ( ( HVSTATUS )( ( FACILITY_HYPERVISOR << 16 ) | ( STATUS_SEVERITY_ERROR << 30 ) | ( 1 << 29 ) | ( 1 ) ) )
#define HVSTATUS_UNSUPPORTED                ( ( HVSTATUS )( ( FACILITY_HYPERVISOR << 16 ) | ( STATUS_SEVERITY_ERROR << 30 ) | ( 1 << 29 ) | ( 2 ) ) )

#define HV_SUCCESS                          NT_SUCCESS

typedef struct _PHYSICAL_REGION_DESCRIPTOR  *PPHYSICAL_REGION_DESCRIPTOR;

typedef struct _VMX_EXIT_TRAP_FRAME         *PVMX_EXIT_TRAP_FRAME;
typedef struct _VMX_SEGMENT_DESCRIPTOR      *PVMX_SEGMENT_DESCRIPTOR;
typedef struct _VMX_PAGE_TABLE_BASE         *PVMX_PAGE_TABLE_BASE;
typedef struct _VMX_PAGE_TABLE              *PVMX_PAGE_TABLE;
typedef struct _VMX_PROCESSOR_STATE         *PVMX_PROCESSOR_STATE;
typedef struct _VMX_EXIT_STATE              *PVMX_EXIT_STATE;
typedef struct _VMX_PCB                     *PVMX_PCB;
typedef HVSTATUS( *PVMX_EXIT_HANDLER )(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
    );

typedef union _VMX_EQ_ACCESS_CONTROL        *PVMX_EQ_ACCESS_CONTROL;
typedef union _VMX_EQ_EPT_VIOLATION         *PVMX_EQ_EPT_VIOLATION;
typedef union _EPT_PML                      *PEPT_PML;

typedef struct _HV_VMM                      *PHV_VMM;

#include "ept.h"
#include "vmx.h"
#include "mp.h"
#include "ex.h"

typedef struct _HV_VMM {
    PVMX_PROCESSOR_STATE ProcessorState;

    ULONG64              MsrMap;
    ULONG64              MsrMapPhysical;

    EPT_POINTER          EptPointer;
    PVMX_PAGE_TABLE_BASE PageTable;

    PLIST_ENTRY          HookHead;
    PLIST_ENTRY          TableHead;
} HV_VMM, *PHV_VMM;

EXTERN HV_VMM g_CurrentMachine;

PFORCEINLINE
PVMX_PCB
HvGetCurrentPcb(

)
{
    return ( PVMX_PCB )__readgsqword( 0 );
}

PFORCEINLINE
ULONG
HvGetCurrentProcessorNumber(

)
{
    return KeGetCurrentProcessorNumber( );//HvGetCurrentPcb( )->Number;
}

#define HvTraceBasic( _, ... )              DbgPrintEx( DPFLTR_IHVDRIVER_ID, 0, "[Limevisor] " _, __VA_ARGS__ )

#define HvGetThreadContext                  __readcr3
#define HvSetThreadContext                  __writecr3

PFORCEINLINE
PVOID
HvGetVirtualAddress(
    __in ULONG64 Address
)
{

    //
    // Shut the fuck up, there is enough address space to waste. (fix tho)
    //

    return MmMapIoSpace( *( PPHYSICAL_ADDRESS )&Address, 0x1000, MmNonCached );
}

PFORCEINLINE
ULONG64
HvGetPhysicalAddress(
    __in PVOID Address
)
{

    return MmGetPhysicalAddress( Address ).QuadPart;
}

PFORCEINLINE
ULONG32
HvAdjustBitControl(
    __in ULONG32 Value,
    __in ULONG32 BitControlMsr
)
{
    ULONG64 BitControl = __readmsr( BitControlMsr );

    Value &= ( ULONG32 )( BitControl >> 32 );
    Value |= ( ULONG32 )( BitControl );

    return Value;
}

HVSTATUS
HvInitializeMachine(

);

HVSTATUS
HvTerminateMachine(

);

NTSTATUS
HvDriverEntry(
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath
);

VOID
HvDriverUnload(
    __in PDRIVER_OBJECT  DriverObject
);
