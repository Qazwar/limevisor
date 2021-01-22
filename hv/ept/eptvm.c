


#include "hv.h"

ULONG64                     g_EptRangeDescriptorsCount;
PHYSICAL_REGION_DESCRIPTOR  g_EptRangeDescriptors[ 10 ];

HVSTATUS
EptInitialize(

)
{

    EptBuildMap( );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
EptInitializeProcessor(
    __in PVMX_PCB Processor
)
{
    //
    //  setup the base page table
    //
    //  irql = dispatch_level
    //

    EPT_POINTER Ept;

    ULONG64 Index3;
    ULONG64 Index2;

    ULONG64 MemoryMappingCount;
    ULONG64 MemoryMappingMax;
    ULONG64 CurrentRange;

    Processor->HookHead = NULL;
    Processor->TableHead = NULL;

    Processor->PageTable = ExAllocatePoolWithTag( NonPagedPool, sizeof( VMX_PAGE_TABLE_BASE ), EPT_POOL_TAG );
    RtlZeroMemory( Processor->PageTable, sizeof( VMX_PAGE_TABLE_BASE ) );

    Processor->PageTable->Level4[ 0 ].PageFrameNumber = HvGetPhysicalAddress( Processor->PageTable->Level3 ) >> 12;
    Processor->PageTable->Level4[ 0 ].ReadAccess = 1;
    Processor->PageTable->Level4[ 0 ].WriteAccess = 1;
    Processor->PageTable->Level4[ 0 ].ExecuteAccess = 1;


    for ( Index3 = 0; Index3 < 512; Index3++ ) {

        Processor->PageTable->Level3[ Index3 ].ReadAccess = 1;
        Processor->PageTable->Level3[ Index3 ].WriteAccess = 1;
        Processor->PageTable->Level3[ Index3 ].ExecuteAccess = 1;
        Processor->PageTable->Level3[ Index3 ].PageFrameNumber = HvGetPhysicalAddress( &Processor->PageTable->Level2[ Index3 ] ) >> 12;
    }

    for ( CurrentRange = 0; CurrentRange < g_EptRangeDescriptorsCount; CurrentRange++ ) {

        MemoryMappingCount = 0;
        MemoryMappingMax = HvBitRound( g_EptRangeDescriptors[ CurrentRange ].RegionLength, 0x200000 );
        //( g_EptRangeDescriptors[ CurrentRange ].RegionLength + 0x1FFFFF ) & ( -0x200000 );

        while ( MemoryMappingCount < MemoryMappingMax ) {

            Index3 = HvIndexLevel3(
                g_EptRangeDescriptors[ CurrentRange ].RegionBase +
                MemoryMappingCount );
            Index2 = HvIndexLevel2(
                g_EptRangeDescriptors[ CurrentRange ].RegionBase +
                MemoryMappingCount );

            Processor->PageTable->Level2[ Index3 ][ Index2 ].MemoryType = g_EptRangeDescriptors[ CurrentRange ].RegionType;
            Processor->PageTable->Level2[ Index3 ][ Index2 ].ReadAccess = 1;
            Processor->PageTable->Level2[ Index3 ][ Index2 ].WriteAccess = 1;
            Processor->PageTable->Level2[ Index3 ][ Index2 ].ExecuteAccess = 1;
            Processor->PageTable->Level2[ Index3 ][ Index2 ].LargePage = 1;
            Processor->PageTable->Level2[ Index3 ][ Index2 ].PageFrameNumber = (
                g_EptRangeDescriptors[ CurrentRange ].RegionBase +
                MemoryMappingCount ) >> 12;

            //DbgPrint( "Mapping %p %d %d\n", g_EptRangeDescriptors[ CurrentRange ].RegionBase + MemoryMappingCount, Index3, Index2 );

            MemoryMappingCount += 0x200000;
        }

    }

    Ept.Long = 0;
    Ept.PageFrameNumber = HvGetPhysicalAddress( Processor->PageTable->Level4 ) >> 12;
    Ept.PageWalkLength = 3;
    Ept.EnableAccessAndDirtyFlags = FALSE;
    Ept.MemoryType = MEMORY_TYPE_WRITE_BACK;

    Processor->EptPointer = Ept;

    return HVSTATUS_SUCCESS;
}

HVSTATUS
EptTerminateProcessor(
    __in PVMX_PCB Processor
)
{
    PLIST_ENTRY Flink;
    PVMX_PAGE_HOOK PageHook;
    PVMX_PAGE_TABLE PageTable;

    ExFreePoolWithTag( Processor->PageTable, EPT_POOL_TAG );

    if ( Processor->HookHead != NULL ) {

        Flink = Processor->HookHead;
        do {
            PageHook = CONTAINING_RECORD( Flink, VMX_PAGE_HOOK, HookLinks );
            Flink = Flink->Flink;
            ExFreePoolWithTag( PageHook, EPT_POOL_TAG );

        } while ( Flink != Processor->HookHead );

        Processor->HookHead = NULL;
    }

    if ( Processor->TableHead != NULL ) {

        Flink = Processor->TableHead;
        do {
            PageTable = CONTAINING_RECORD( Flink, VMX_PAGE_TABLE, TableLinks );
            Flink = Flink->Flink;
            ExFreePoolWithTag( PageTable, EPT_POOL_TAG );

        } while ( Flink != Processor->TableHead );

        Processor->TableHead = NULL;
    }

    return HVSTATUS_SUCCESS;
}

VOID
EptBuildMap(

)
{

    ULONG64 CurrentRegister;
    ULONG64 BitCount;

    PPHYSICAL_REGION_DESCRIPTOR CurrentDescriptor;

    ULONG64 MtrrCapabilities;
    ULONG64 PhysicalBase;
    ULONG64 PhysicalMask;

    MtrrCapabilities = __readmsr( IA32_MTRR_CAPABILITIES );

    for ( CurrentRegister = 0; CurrentRegister < ( ( ULONG64 )( UCHAR )MtrrCapabilities ); CurrentRegister++ ) {

        PhysicalBase = __readmsr( ( ULONG32 )( IA32_MTRR_PHYSBASE0 + CurrentRegister * 2 ) );
        PhysicalMask = __readmsr( ( ULONG32 )( IA32_MTRR_PHYSMASK0 + CurrentRegister * 2 ) );

        //
        //  check valid bit.
        //

        if ( PhysicalMask & ( 1 << 11 ) ) {

            CurrentDescriptor = &g_EptRangeDescriptors[ g_EptRangeDescriptorsCount++ ];
            CurrentDescriptor->RegionBase = PhysicalBase & 0xFFFFFFFFF000;

            _BitScanForward64( ( ULONG* )&BitCount, PhysicalMask & 0xFFFFFFFFF000 );
            CurrentDescriptor->RegionLength = ( 1ULL << BitCount ) - 1;

            CurrentDescriptor->RegionType = ( ULONG64 )( UCHAR )PhysicalBase;

            HvTraceBasic( "MTRR Range: %p %p %p %x\n",
                          CurrentDescriptor->RegionBase,
                          CurrentDescriptor->RegionBase + CurrentDescriptor->RegionLength,
                          CurrentDescriptor->RegionLength,
                          CurrentDescriptor->RegionType );
        }
    }

    return;
}

