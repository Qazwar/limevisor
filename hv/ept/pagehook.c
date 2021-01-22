


#include "hv.h"

//
//  this procedure wants all parameters to be virtual addresses and
//  for them to have been aligned on a page boundary before-hand.
//  
//  PageOriginal    should specify the base address of the original memory.
//  PageHook        should specify the base address of the hook memory.
//  PageLength      should specify the width of the hook, and how many pages we're
//                  actually hooking.
//  Behaviour       this is a bit flag from the definitions above, it tells the
//                  fault handler how to interpret an ept violation at this 
//                  address.
//

HVSTATUS
EptInstallPageHook(
    __in ULONG64 PageOriginal,
    __in ULONG64 PageHook,
    __in ULONG64 PageLength,
    __in ULONG32 Behaviour
)
{
    PVMX_PAGE_HOOK PageHookEntry;

    PageHookEntry = ExAllocatePoolWithTag(
        NonPagedPoolNx,
        sizeof( VMX_PAGE_HOOK ) + PageLength * sizeof( *PageHookEntry->PagePhysical ),
        EPT_POOL_TAG );

    PageHookEntry->PageOriginal = PageOriginal;
    PageHookEntry->PageHook = PageHook;
    PageHookEntry->PageLength = PageLength;
    PageHookEntry->Behaviour = Behaviour;

    if ( g_CurrentMachine.HookHead == NULL ) {

        InitializeListHead( &PageHookEntry->HookLinks );
        g_CurrentMachine.HookHead = &PageHookEntry->HookLinks;
    }
    else {

        InsertTailList( g_CurrentMachine.HookHead, &PageHookEntry->HookLinks );
    }

    for ( ULONG64 Page = 0; Page < PageHookEntry->PageLength; Page++ ) {

        PageHookEntry->PagePhysical[ Page ].PageHook =
            HvGetPhysicalAddress(
            ( PVOID )( PageHookEntry->PageHook + Page * 0x1000 ) );
        PageHookEntry->PagePhysical[ Page ].PageOriginal =
            HvGetPhysicalAddress(
            ( PVOID )( PageHookEntry->PageOriginal + Page * 0x1000 ) );
        PageHookEntry->PagePhysical[ Page ].PageEntry =
            EptAddressPageEntry( PageHookEntry->PagePhysical[ Page ].PageOriginal );

        PageHookEntry->PagePhysical[ Page ].PageEntry->PageFrameNumber = PageHookEntry->PagePhysical[ Page ].PageHook / 0x1000;

        switch ( PageHookEntry->Behaviour ) {
        case EPT_HOOK_BEHAVIOUR_EXECUTE:;
            PageHookEntry->PagePhysical[ Page ].PageEntry->ReadAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PageEntry->WriteAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PageEntry->ExecuteAccess = 1;
            break;
        case EPT_HOOK_BEHAVIOUR_READWRITE:;
            PageHookEntry->PagePhysical[ Page ].PageEntry->ReadAccess = 1;
            PageHookEntry->PagePhysical[ Page ].PageEntry->WriteAccess = 1;
            PageHookEntry->PagePhysical[ Page ].PageEntry->ExecuteAccess = 0;
            break;
        case EPT_HOOK_BEHAVIOUR_HIDE:;
            PageHookEntry->PagePhysical[ Page ].PageEntry->ReadAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PageEntry->WriteAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PageEntry->ExecuteAccess = 1;
            break;
        default:
            break;
        }
    }

    return HVSTATUS_SUCCESS;
}

//
//  this function is called by EptViolationFault if
//  the address accessed is inside a hook region.
//  this function should deal with the hook and handle it 
//  appropriately, setting the correct access based on behaviour.
//

HVSTATUS
EptPageHookFault(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState,
    __in ULONG64              AddressAccessed
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    PLIST_ENTRY Flink;
    PVMX_PAGE_HOOK PageHook;

    VMX_EQ_EPT_VIOLATION Qualification;


    Qualification.Long = ExitState->ExitQualification;

    Flink = g_CurrentMachine.HookHead;
    do {
        PageHook = CONTAINING_RECORD( Flink, VMX_PAGE_HOOK, HookLinks );

        for ( ULONG64 Page = 0; Page < PageHook->PageLength; Page++ ) {

            if ( AddressAccessed >= PageHook->PagePhysical[ Page ].PageOriginal &&
                 AddressAccessed <= PageHook->PagePhysical[ Page ].PageOriginal + 0xFFF ) {

                HvTraceBasic( "page hook access %p\n", AddressAccessed );

                if ( PageHook->PagePhysical[ Page ].PageEntry->PageFrameNumber == PageHook->PagePhysical[ Page ].PageHook / 0x1000 ) {

                    PageHook->PagePhysical[ Page ].PageEntry->PageFrameNumber = PageHook->PagePhysical[ Page ].PageOriginal / 0x1000;
                }
                else {

                    PageHook->PagePhysical[ Page ].PageEntry->PageFrameNumber = PageHook->PagePhysical[ Page ].PageHook / 0x1000;
                }

                PageHook->PagePhysical[ Page ].PageEntry->ReadAccess = !PageHook->PagePhysical[ Page ].PageEntry->ReadAccess;
                PageHook->PagePhysical[ Page ].PageEntry->WriteAccess = !PageHook->PagePhysical[ Page ].PageEntry->WriteAccess;
                PageHook->PagePhysical[ Page ].PageEntry->ExecuteAccess = !PageHook->PagePhysical[ Page ].PageEntry->ExecuteAccess;

                ExitState->IncrementIp = FALSE;

                return HVSTATUS_SUCCESS;
            }
        }

        Flink = Flink->Flink;
    } while ( Flink != g_CurrentMachine.HookHead );

    return HVSTATUS_UNSUCCESSFUL;
}
