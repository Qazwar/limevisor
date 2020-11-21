


#pragma once

typedef union _EPT_POINTER {
    struct {
        ULONG64 MemoryType : 3;
        ULONG64 PageWalkLength : 3; // -1
        ULONG64 EnableAccessAndDirtyFlags : 1;
        ULONG64 Reserved1 : 5;
        ULONG64 PageFrameNumber : 36;
        ULONG64 Reserved2 : 16;
    };

    ULONG64 Value;
} EPT_POINTER, *PEPT_POINTER;

typedef union _EPT_PML4 {
    struct {
        ULONG64 ReadAccess : 1;
        ULONG64 WriteAccess : 1;
        ULONG64 ExecuteAccess : 1;
        ULONG64 Reserved1 : 5;
        ULONG64 Accessed : 1;
        ULONG64 Reserved2 : 1;
        ULONG64 UserModeExecute : 1;
        ULONG64 Reserved3 : 1;
        ULONG64 PageFrameNumber : 36;
        ULONG64 Reserved4 : 16;
    };

    ULONG64 Value;
} EPT_PML4, *PEPT_PML4;

typedef union _EPT_PML3_LARGE {
    struct {
        ULONG64 ReadAccess : 1;
        ULONG64 WriteAccess : 1;
        ULONG64 ExecuteAccess : 1;
        ULONG64 MemoryType : 3;
        ULONG64 IgnorePat : 1;
        ULONG64 LargePage : 1; // 1 = EPT_PML3_LARGE, 0 = EPT_PML3
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;
        ULONG64 UserModeExecute : 1;
        ULONG64 Reserved1 : 19;
        ULONG64 PageFrameNumber : 18;
        ULONG64 Reserved2 : 15;
        ULONG64 SuppressVe : 1;
    };

    ULONG64 Value;
} EPT_PML3_LARGE, *PEPT_PML3_LARGE;

typedef union _EPT_PML3 {
    struct {
        ULONG64 ReadAccess : 1;
        ULONG64 WriteAccess : 1;
        ULONG64 ExecuteAccess : 1;
        ULONG64 Reserved1 : 4;
        ULONG64 LargePage : 1; // 1 = EPT_PML3_LARGE, 0 = EPT_PML3
        ULONG64 Accessed : 1;
        ULONG64 Reserved2 : 1;
        ULONG64 UserModeExecute : 1;
        ULONG64 Reserved3 : 1;
        ULONG64 PageFrameNumber : 36;
        ULONG64 Reserved4 : 16;
    };

    ULONG64 Value;
} EPT_PML3, *PEPT_PML3;

typedef union _EPT_PML2_LARGE {
    struct {
        ULONG64 ReadAccess : 1;
        ULONG64 WriteAccess : 1;
        ULONG64 ExecuteAccess : 1;
        ULONG64 MemoryType : 3;
        ULONG64 IgnorePat : 1;
        ULONG64 LargePage : 1; // 1 = EPT_PML2_LARGE, 0 = EPT_PML2
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;
        ULONG64 UserModeExecute : 1;
        ULONG64 Reserved1 : 10;
        ULONG64 PageFrameNumber : 27;
        ULONG64 Reserved2 : 15;
        ULONG64 SuppressVe : 1;
    };

    ULONG64 Value;
} EPT_PML2_LARGE, *PEPT_PML2_LARGE;

//
//  TODO:   consider merging LARGE structures and
//          non large structures using a union.
//

typedef union _EPT_PML2 {
    struct {
        ULONG64 ReadAccess : 1;
        ULONG64 WriteAccess : 1;
        ULONG64 ExecuteAccess : 1;
        ULONG64 Reserved1 : 4;
        ULONG64 LargePage : 1; // 1 = EPT_PML2_LARGE, 0 = EPT_PML2
        ULONG64 Accessed : 1;
        ULONG64 Reserved2 : 1;
        ULONG64 UserModeExecute : 1;
        ULONG64 Reserved3 : 1;
        ULONG64 PageFrameNumber : 36;
        ULONG64 Reserved4 : 16;
    };

    ULONG64 Value;
} EPT_PML2, *PEPT_PML2;

typedef union _EPT_PML1 {
    struct {
        ULONG64 ReadAccess : 1;
        ULONG64 WriteAccess : 1;
        ULONG64 ExecuteAccess : 1;
        ULONG64 MemoryType : 3;
        ULONG64 IgnorePat : 1;
        ULONG64 Reserved1 : 1;
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;
        ULONG64 UserModeExecute : 1;
        ULONG64 Reserved2 : 1;
        ULONG64 PageFrameNumber : 36;
        ULONG64 Reserved3 : 15;
        ULONG64 SuppressVe : 1;
    };

    ULONG64 Value;
} EPT_PML1, *PEPT_PML1;

C_ASSERT( sizeof( EPT_PML4 ) == 8 );
C_ASSERT( sizeof( EPT_PML3 ) == 8 );
C_ASSERT( sizeof( EPT_PML3_LARGE ) == 8 );
C_ASSERT( sizeof( EPT_PML2 ) == 8 );
C_ASSERT( sizeof( EPT_PML2_LARGE ) == 8 );
C_ASSERT( sizeof( EPT_PML1 ) == 8 );

#define PML4_INDEX( address ) ( ( ( address ) & 0xFF8000000000ULL )  >> 39 )
#define PML3_INDEX( address ) ( ( ( address ) & 0x7FC0000000ULL )  >> 30 )
#define PML2_INDEX( address ) ( ( ( address ) & 0x3FE00000ULL )  >> 21 )
#define PML1_INDEX( address ) ( ( ( address ) & 0x1FF000ULL )  >> 12 )

#define MEMORY_TYPE_UNCACHEABLE                                      0x00000000
#define MEMORY_TYPE_WRITE_COMBINING                                  0x00000001
#define MEMORY_TYPE_WRITE_THROUGH                                    0x00000004
#define MEMORY_TYPE_WRITE_PROTECTED                                  0x00000005
#define MEMORY_TYPE_WRITE_BACK                                       0x00000006
#define MEMORY_TYPE_INVALID                                          0x000000FF

#define EPT_POOL_TAG ' TPE'

typedef union _VMX_EXIT_QUALIFICATION_EPT_VIOLATION {
    struct {
        ULONG64 ReadAccess : 1;
        ULONG64 WriteAccess : 1;
        ULONG64 ExecuteAccess : 1;
        ULONG64 Readable : 1;
        ULONG64 Writeable : 1;
        ULONG64 Executable : 1;
        ULONG64 ExecutableForUserMode : 1;
        ULONG64 ValidGuestLinearAddress : 1;
        ULONG64 CausedByTranslation : 1;
        ULONG64 UserModeLinearAddress : 1;
        ULONG64 ReadableWriteablePage : 1;
        ULONG64 ExecuteDisablePage : 1;
        ULONG64 NmiUnblocking : 1;
        ULONG64 Reserved1 : 51;
    };

    ULONG64 Value;
} VMX_EXIT_QUALIFICATION_EPT_VIOLATION, *PVMX_EXIT_QUALIFICATION_EPT_VIOLATION;

#pragma push(pack, 8)
typedef struct _VMX_PAGE_HOOK {
    LIST_ENTRY      HookLinks;

    //
    //  the virtual address bases and hook behaviour
    //

    ULONG64         PageOriginal;
    ULONG64         PageHook;
    ULONG32         Behaviour;

    //
    //  this parameter is used to judge the length
    //  of PagePhysical and the length of memory
    //  that is hooked.
    //

    ULONG64         PageLength;

    struct {
        //
        //  PageOriginal & PageHook are physical addresses
        //  whilst PML1, is a the virtual address of the PML1 entry.
        //

        ULONG64     PageOriginal;
        ULONG64     PageHook;
        PEPT_PML1   PML1;

    } PagePhysical[ 0 ];

} VMX_PAGE_HOOK, *PVMX_PAGE_HOOK;
#pragma pop(pack)

HVSTATUS
EptInitialize(

);

VOID
EptInitializePhysicalMap(

);

HVSTATUS
EptInitializeProcessor(
    __in PVMX_PROCESSOR_STATE ProcessorState
);

HVSTATUS
EptTerminateProcessor(
    __in PVMX_PROCESSOR_STATE ProcessorState
);

HVSTATUS
EptViolationFault(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
);

PEPT_PML1
EptAddressPageEntry(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in ULONG64                PhysicalAddress
);

HVSTATUS
EptPageHookFault(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState,
    __in ULONG64                AddressAccessed
);

#define EPT_HOOK_BEHAVIOUR_EXECUTE      ( ( ULONG32 )0x00000000L ) //PageHook execute   PageOriginal read/write
#define EPT_HOOK_BEHAVIOUR_READ         ( ( ULONG32 )0x00000001L ) //PageHook read      PageOriginal execute/write
#define EPT_HOOK_BEHAVIOUR_WRITE        ( ( ULONG32 )0x00000002L ) //PageHook write     PageOriginal execute/read
#define EPT_HOOK_BEHAVIOUR_READWRITE    ( ( ULONG32 )0x00000004L ) //PageHook readwrite PageOriginal execute

//
//  with this one, PageHook is used as a physical buffer
//  for any foreign thread read or writing it will be redirected
//  to this, however, any read/write from an Ip inside the 
//  PageOriginal + PageLength will be allowed.
//
//  TODO:   consider adding function's to temporarily disable this
//          such that if NtCreateFile wants to write to the IoStatusBlock
//          it can, and without creating a larger allocation via
//          ExAllocatePoolWithTag or MmAllocatedNonCachedMemory..
//

#define EPT_HOOK_BEHAVIOUR_HIDE         ( ( ULONG32 )0x00000008L ) 

HVSTATUS
EptInstallPageHook(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in ULONG64                PageOriginal,
    __in ULONG64                PageHook,
    __in ULONG64                PageLength,
    __in ULONG32                Behaviour
);