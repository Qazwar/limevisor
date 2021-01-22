


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

    ULONG64     Long;
} EPT_POINTER, *PEPT_POINTER;

typedef union _EPT_PML {
    struct {
        ULONG64 ReadAccess : 1;
        ULONG64 WriteAccess : 1;
        ULONG64 ExecuteAccess : 1;
        ULONG64 MemoryType : 3;
        ULONG64 IgnorePat : 1;
        ULONG64 LargePage : 1;
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;
        ULONG64 UserModeExecute : 1;
        ULONG64 Reserved2 : 1;
        ULONG64 PageFrameNumber : 36;
        ULONG64 Reserved3 : 15;
        ULONG64 SuppressVe : 1;
    };

    ULONG64     Long;
} EPT_PML, *PEPT_PML;

C_ASSERT( sizeof( EPT_PML ) == 8 );

#define HvIndexLevel4( address )                ( ( ( ULONG64 ) ( address ) & ( 0x1FFULL << 39ULL ) ) >> 39ULL )
#define HvIndexLevel3( address )                ( ( ( ULONG64 ) ( address ) & ( 0x1FFULL << 30ULL ) ) >> 30ULL )
#define HvIndexLevel2( address )                ( ( ( ULONG64 ) ( address ) & ( 0x1FFULL << 21ULL ) ) >> 21ULL )
#define HvIndexLevel1( address )                ( ( ( ULONG64 ) ( address ) & ( 0x1FFULL << 12ULL ) ) >> 12ULL )

#define HvConstructAddress( index4, index3, index2, index1 ) \
( ( PVOID )( ( ( ULONG64 )( index4 ) << 39ULL ) |\
( ( ULONG64 )( index3 ) << 30ULL ) |\
( ( ULONG64 )( index2 ) << 21ULL ) |\
( ( ULONG64 )( index1 ) << 12ULL ) |\
( ( ( ULONG64 )( index4 ) / 256 ) * 0xFFFF000000000000 ) ) )

#define MEMORY_TYPE_UNCACHEABLE                                      0x00000000
#define MEMORY_TYPE_WRITE_COMBINING                                  0x00000001
#define MEMORY_TYPE_WRITE_THROUGH                                    0x00000004
#define MEMORY_TYPE_WRITE_PROTECTED                                  0x00000005
#define MEMORY_TYPE_WRITE_BACK                                       0x00000006
#define MEMORY_TYPE_INVALID                                          0x000000FF

#define EPT_POOL_TAG ' TPE'

typedef union _VMX_EQ_EPT_VIOLATION {
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

    ULONG64 Long;
} VMX_EQ_EPT_VIOLATION, *PVMX_EQ_EPT_VIOLATION;

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
        PEPT_PML    PageEntry;

    } PagePhysical[ 0 ];

} VMX_PAGE_HOOK, *PVMX_PAGE_HOOK;
#pragma pop(pack)

HVSTATUS
EptInitialize(

);

VOID
EptBuildMap(

);

HVSTATUS
EptInitializeMachine(

);

HVSTATUS
EptTerminateMachine(

);

HVSTATUS
EptViolationFault(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

PEPT_PML
EptAddressPageEntry(
    __in ULONG64 PhysicalAddress
);

HVSTATUS
EptPageHookFault(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState,
    __in ULONG64              AddressAccessed
);

#define EPT_HOOK_BEHAVIOUR_EXECUTE      ( ( ULONG32 )0x00000000L ) //PageHook execute   PageOriginal read/write
#define EPT_HOOK_BEHAVIOUR_READWRITE    ( ( ULONG32 )0x00000001L ) //PageHook readwrite PageOriginal execute

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

#define EPT_HOOK_BEHAVIOUR_HIDE         ( ( ULONG32 )0x00000002L ) 

HVSTATUS
EptInstallPageHook(
    __in ULONG64 PageOriginal,
    __in ULONG64 PageHook,
    __in ULONG64 PageLength,
    __in ULONG32 Behaviour
);