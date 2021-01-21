


#include "hv.h"

PVMX_PAGE_TABLE
EptDecomposeLargeLevel2(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PEPT_PML               LargePage
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

    PHYSICAL_ADDRESS Max = { ~0ULL };

    PageTable = MmAllocateContiguousMemory( sizeof( VMX_PAGE_TABLE ), Max );//ExAllocatePoolWithTag( NonPagedPool, sizeof( VMX_PAGE_TABLE ), EPT_POOL_TAG );
    RtlZeroMemory( PageTable, sizeof( VMX_PAGE_TABLE ) );

    PageTable->PageEntryParent = LargePage;
    PageTable->PhysicalBaseAddress = LargePage->PageFrameNumber << 12;

    for ( ULONG64 CurrentEntry = 0; CurrentEntry < 512; CurrentEntry++ ) {

        PageTable->PageEntry[ CurrentEntry ].Long = 0;
        PageTable->PageEntry[ CurrentEntry ].MemoryType = LargePage->MemoryType;
        PageTable->PageEntry[ CurrentEntry ].IgnorePat = LargePage->IgnorePat;
        PageTable->PageEntry[ CurrentEntry ].SuppressVe = LargePage->SuppressVe;
        PageTable->PageEntry[ CurrentEntry ].ReadAccess = 1;
        PageTable->PageEntry[ CurrentEntry ].WriteAccess = 1;
        PageTable->PageEntry[ CurrentEntry ].ExecuteAccess = 1;
        PageTable->PageEntry[ CurrentEntry ].PageFrameNumber = ( PageTable->PhysicalBaseAddress + ( CurrentEntry << 12 ) ) >> 12;
        HvTraceBasic( "PML1 decomposure %p\n", PageTable->PhysicalBaseAddress + ( CurrentEntry << 12 ) );
    }

    LargePage->Long = 0;
    LargePage->PageFrameNumber = HvGetPhysicalAddress( PageTable ) >> 12;
    LargePage->ReadAccess = 1;
    LargePage->WriteAccess = 1;
    LargePage->ExecuteAccess = 1;

    if ( ProcessorState->TableHead == NULL ) {

        InitializeListHead( &PageTable->TableLinks );
        ProcessorState->TableHead = &PageTable->TableLinks;
    }
    else {

        InsertHeadList( ProcessorState->TableHead, &PageTable->TableLinks );
    }

    return PageTable;
}

PEPT_PML
EptAddressPageEntry(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in ULONG64                PhysicalAddress
)
{
    PLIST_ENTRY TableFlink;

    PEPT_PML LargePage;
    PVMX_PAGE_TABLE PageTable;

    LargePage = &ProcessorState->PageTable->Level2[ HvIndexLevel3( PhysicalAddress ) ][ HvIndexLevel2( PhysicalAddress ) ];

    if ( LargePage->LargePage ) {

        PageTable = EptDecomposeLargeLevel2( ProcessorState, LargePage );
        DbgBreakPoint( );
        return &PageTable->PageEntry[ HvIndexLevel1( PhysicalAddress ) ];
    }
    else {

        //
        //  this page isn't large, search for it's entry
        //  inside our doubly linked list of page tables
        //

        TableFlink = ProcessorState->TableHead;

        do {
            PageTable = CONTAINING_RECORD( TableFlink, VMX_PAGE_TABLE, TableLinks );

            if ( PhysicalAddress >= PageTable->PhysicalBaseAddress &&
                 PhysicalAddress < PageTable->PhysicalBaseAddress + 0x200000 ) {

                return &PageTable->PageEntry[ HvIndexLevel1( PhysicalAddress ) ];
            }

            TableFlink = TableFlink->Flink;
        } while ( TableFlink != ProcessorState->TableHead );

        //
        //  this is bad lol
        //

        KeBugCheck( HYPERVISOR_ERROR );
    }

}