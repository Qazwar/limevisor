


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

#undef  RtlZeroMemory
#define RtlZeroMemory( dst, size )           memset( ( void* )( dst ), 0, ( size_t )( size ) )

#undef  C_ASSERT
#define C_ASSERT( e )                       static_assert( e, "ASSERTION FAILURE" )

#define EXTERN                              extern

typedef ULONG32 HVSTATUS;

#define HVSTATUS_SUCCESS                    ( ( HVSTATUS )0x00000000L )
#define HVSTATUS_UNSUCCESSFUL               ( ( HVSTATUS )0x00000001L )
#define HVSTATUS_UNSUPPORTED                ( ( HVSTATUS )0x00000002L )

#define HV_SUCCESS( status )                ( ( ( HVSTATUS )( status ) ) == ( HVSTATUS_SUCCESS ) )

typedef struct _VMX_EXIT_TRAP_FRAME         *PVMX_EXIT_TRAP_FRAME;
typedef struct _VMX_SEGMENT_DESCRIPTOR      *PVMX_SEGMENT_DESCRIPTOR;
typedef struct _PHYSICAL_REGION_DESCRIPTOR  *PPHYSICAL_REGION_DESCRIPTOR;
typedef struct _VMX_PAGE_TABLE_BASE         *PVMX_PAGE_TABLE_BASE;
typedef struct _VMX_PAGE_TABLE              *PVMX_PAGE_TABLE;
typedef struct _VMX_PROCESSOR_STATE         *PVMX_PROCESSOR_STATE;
typedef struct _VMX_EXIT_STATE              *PVMX_EXIT_STATE;
typedef HVSTATUS( *PVMX_EXIT_HANDLER )(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
    );

typedef union _VMX_EXIT_QUALIFICATION_MOV_CR    *PVMX_EXIT_QUALIFICATION_MOV_CR;

typedef union _EPT_POINTER                  *PEPT_POINTER;
typedef union _EPT_PML4                     *PEPT_PML4;
typedef union _EPT_PML3                     *PEPT_PML3;
typedef union _EPT_PML3_LARGE               *PEPT_PML3_LARGE;
typedef union _EPT_PML2                     *PEPT_PML2;
typedef union _EPT_PML2_LARGE               *PEPT_PML2_LARGE;
typedef union _EPT_PML1                     *PEPT_PML1;

#include "ept.h"
#include "vmx.h"
#include "mp.h"
#include "ex.h"

EXTERN PVMX_PROCESSOR_STATE g_ProcessorState;

#define HvTraceBasic( _, ... )              DbgPrint( "[Limevisor] " _, __VA_ARGS__ )

#define HvGetThreadContext                  __readcr3
#define HvSetThreadContext                  __writecr3

PFORCEINLINE
PVOID
HvGetVirtualAddress(
    __in ULONG64 Address
)
{

    //
    //  Shut the fuck up.
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
PVMX_PROCESSOR_STATE
HvGetProcessorState(

)
{

    return &g_ProcessorState[ KeGetCurrentProcessorNumber( ) ];
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
HvInitializeVisor(

);

HVSTATUS
HvTerminateVisor(

);

NTSTATUS
HvDriverEntry(
    __in PDRIVER_OBJECT     DriverObject,
    __in PUNICODE_STRING    RegistryPath
);

VOID
HvDriverUnload(
    __in PDRIVER_OBJECT     DriverObject
);
