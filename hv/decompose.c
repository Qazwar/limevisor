


#include "hv.h"

PVMX_PAGE_TABLE
EptDecomposeLargeEntry(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PEPT_PML2_LARGE        PML2Large
)
{
    //
    //  decomposes a large page table entry into a smaller
    //  set of page tables.
    //
    //  TODO:   consider adding support to select 
    //          whether its a 1GB PML3, or a 2MB PML2
    //
    //  irql <= dispatch_level
    //

    PVMX_PAGE_TABLE PageTable;

    PEPT_PML2 PML2;

    PHYSICAL_ADDRESS Max = { ~0ULL };

    PML2 = ( PEPT_PML2 )PML2Large;

    PageTable = MmAllocateContiguousMemory( sizeof( VMX_PAGE_TABLE ), Max );//ExAllocatePoolWithTag( NonPagedPool, sizeof( VMX_PAGE_TABLE ), EPT_POOL_TAG );
    RtlZeroMemory( PageTable, sizeof( VMX_PAGE_TABLE ) );

    PageTable->PML2 = PML2;
    PageTable->PhysicalBaseAddress = PML2Large->PageFrameNumber * 0x200000;

    for (ULONG64 CurrentEntry = 0; CurrentEntry < 512; CurrentEntry++) {

        PageTable->PML1[ CurrentEntry ].Value = 0;
        PageTable->PML1[ CurrentEntry ].MemoryType = PML2Large->MemoryType;
        PageTable->PML1[ CurrentEntry ].IgnorePat = PML2Large->IgnorePat;
        PageTable->PML1[ CurrentEntry ].SuppressVe = PML2Large->SuppressVe;
        PageTable->PML1[ CurrentEntry ].ReadAccess = 1;
        PageTable->PML1[ CurrentEntry ].WriteAccess = 1;
        PageTable->PML1[ CurrentEntry ].ExecuteAccess = 1;
        PageTable->PML1[ CurrentEntry ].PageFrameNumber = ( PageTable->PhysicalBaseAddress + CurrentEntry * 0x1000 ) / 0x1000;
        HvTraceBasic( "PML1 decomposure %p\n", ( PageTable->PhysicalBaseAddress + CurrentEntry * 0x1000 ) );
    }

    PML2->Value = 0;
    PML2->PageFrameNumber = HvGetPhysicalAddress( PageTable ) / 0x1000;
    PML2->ReadAccess = 1;
    PML2->WriteAccess = 1;
    PML2->ExecuteAccess = 1;

    if (ProcessorState->TableHead == NULL) {

        InitializeListHead( &PageTable->TableLinks );
        ProcessorState->TableHead = &PageTable->TableLinks;
    }
    else {

        InsertHeadList( ProcessorState->TableHead, &PageTable->TableLinks );
    }

    return PageTable;
}

PEPT_PML1
EptAddressPageEntry(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in ULONG64                PhysicalAddress
)
{
    PLIST_ENTRY TableFlink;

    PEPT_PML2_LARGE PML2Large;
    PVMX_PAGE_TABLE PageTable;

    PML2Large = &ProcessorState->PageTable->PML2Large[ PML3_INDEX( PhysicalAddress ) ][ PML2_INDEX( PhysicalAddress ) ];

    if (PML2Large->LargePage) {

        PageTable = EptDecomposeLargeEntry( ProcessorState, PML2Large );
        DbgBreakPoint( );
        return &PageTable->PML1[ PML1_INDEX( PhysicalAddress ) ];
    }
    else {

        //
        //  this page isn't large, search for it's entry
        //  inside our doubly linked list of page tables
        //

        TableFlink = ProcessorState->TableHead;

        do {
            PageTable = CONTAINING_RECORD( TableFlink, VMX_PAGE_TABLE, TableLinks );

            if (PhysicalAddress >= PageTable->PhysicalBaseAddress &&
                PhysicalAddress < PageTable->PhysicalBaseAddress + 0x200000) {

                return &PageTable->PML1[ PML1_INDEX( PhysicalAddress ) ];
            }

            TableFlink = TableFlink->Flink;
        } while (TableFlink != ProcessorState->TableHead);

        //
        //  this is bad lol
        //

        KeBugCheck( HYPERVISOR_ERROR );
    }

}