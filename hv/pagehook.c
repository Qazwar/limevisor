


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
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in ULONG64                PageOriginal,
    __in ULONG64                PageHook,
    __in ULONG64                PageLength,
    __in ULONG32                Behaviour
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

    if (ProcessorState->HookHead == NULL) {

        InitializeListHead( &PageHookEntry->HookLinks );
        ProcessorState->HookHead = &PageHookEntry->HookLinks;
    }
    else {

        InsertHeadList( ProcessorState->HookHead, &PageHookEntry->HookLinks );
    }

    for (ULONG64 Page = 0; Page < PageHookEntry->PageLength; Page++) {

        PageHookEntry->PagePhysical[ Page ].PageHook =
            HvGetPhysicalAddress(
            ( PVOID )( PageHookEntry->PageHook + Page * 0x1000 ) );
        PageHookEntry->PagePhysical[ Page ].PageOriginal =
            HvGetPhysicalAddress(
            ( PVOID )( PageHookEntry->PageOriginal + Page * 0x1000 ) );
        PageHookEntry->PagePhysical[ Page ].PML1 =
            EptAddressPageEntry(
            ProcessorState,
            PageHookEntry->PagePhysical[ Page ].PageOriginal );

        PageHookEntry->PagePhysical[ Page ].PML1->PageFrameNumber = PageHookEntry->PagePhysical[ Page ].PageHook / 0x1000;

        switch (PageHookEntry->Behaviour) {
        case EPT_HOOK_BEHAVIOUR_EXECUTE:;
            PageHookEntry->PagePhysical[ Page ].PML1->ReadAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PML1->WriteAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PML1->ExecuteAccess = 1;
            break;
        case EPT_HOOK_BEHAVIOUR_READ:;
            PageHookEntry->PagePhysical[ Page ].PML1->ReadAccess = 1;
            PageHookEntry->PagePhysical[ Page ].PML1->WriteAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PML1->ExecuteAccess = 0;
            break;
        case EPT_HOOK_BEHAVIOUR_WRITE:;
            PageHookEntry->PagePhysical[ Page ].PML1->ReadAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PML1->WriteAccess = 1;
            PageHookEntry->PagePhysical[ Page ].PML1->ExecuteAccess = 0;
            break;
        case EPT_HOOK_BEHAVIOUR_READWRITE:;
            PageHookEntry->PagePhysical[ Page ].PML1->ReadAccess = 1;
            PageHookEntry->PagePhysical[ Page ].PML1->WriteAccess = 1;
            PageHookEntry->PagePhysical[ Page ].PML1->ExecuteAccess = 0;
            break;
        case EPT_HOOK_BEHAVIOUR_HIDE:;
            PageHookEntry->PagePhysical[ Page ].PML1->ReadAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PML1->WriteAccess = 0;
            PageHookEntry->PagePhysical[ Page ].PML1->ExecuteAccess = 1;
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
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState,
    __in ULONG64                AddressAccessed
)
{
    TrapFrame;
    ExitState;

    PLIST_ENTRY Flink;
    PVMX_PAGE_HOOK PageHook;

    VMX_EXIT_QUALIFICATION_EPT_VIOLATION Qualification;


    Qualification.Value = ExitState->ExitQualification;

    Flink = ProcessorState->HookHead;
    do {
        PageHook = CONTAINING_RECORD( Flink, VMX_PAGE_HOOK, HookLinks );

        for (ULONG64 Page = 0; Page < PageHook->PageLength; Page++) {

            if (AddressAccessed >= PageHook->PagePhysical[ Page ].PageOriginal &&
                AddressAccessed <= PageHook->PagePhysical[ Page ].PageOriginal + 0xFFF) {

                HvTraceBasic( "Page hook access %p\n", AddressAccessed );

                if (PageHook->PagePhysical[ Page ].PML1->PageFrameNumber == PageHook->PagePhysical[ Page ].PageHook / 0x1000) {

                    PageHook->PagePhysical[ Page ].PML1->PageFrameNumber = PageHook->PagePhysical[ Page ].PageOriginal / 0x1000;
                }
                else {

                    PageHook->PagePhysical[ Page ].PML1->PageFrameNumber = PageHook->PagePhysical[ Page ].PageHook / 0x1000;
                }

                PageHook->PagePhysical[ Page ].PML1->ReadAccess = !PageHook->PagePhysical[ Page ].PML1->ReadAccess;
                PageHook->PagePhysical[ Page ].PML1->WriteAccess = !PageHook->PagePhysical[ Page ].PML1->WriteAccess;
                PageHook->PagePhysical[ Page ].PML1->ExecuteAccess = !PageHook->PagePhysical[ Page ].PML1->ExecuteAccess;

                ExitState->IncrementIp = FALSE;

                return HVSTATUS_SUCCESS;
            }
        }

        Flink = Flink->Flink;
    } while (Flink != ProcessorState->HookHead);

    return HVSTATUS_UNSUCCESSFUL;
}