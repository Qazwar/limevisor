


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
EptInitializeMachine(

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

    g_CurrentMachine.HookHead = NULL;
    g_CurrentMachine.TableHead = NULL;

    g_CurrentMachine.PageTable = ExAllocatePoolWithTag( NonPagedPool, sizeof( VMX_PAGE_TABLE_BASE ), EPT_POOL_TAG );
    RtlZeroMemory( g_CurrentMachine.PageTable, sizeof( VMX_PAGE_TABLE_BASE ) );

    g_CurrentMachine.PageTable->Level4[ 0 ].PageFrameNumber = HvGetPhysicalAddress( g_CurrentMachine.PageTable->Level3 ) >> 12;
    g_CurrentMachine.PageTable->Level4[ 0 ].ReadAccess = 1;
    g_CurrentMachine.PageTable->Level4[ 0 ].WriteAccess = 1;
    g_CurrentMachine.PageTable->Level4[ 0 ].ExecuteAccess = 1;


    for ( Index3 = 0; Index3 < 512; Index3++ ) {

        g_CurrentMachine.PageTable->Level3[ Index3 ].ReadAccess = 1;
        g_CurrentMachine.PageTable->Level3[ Index3 ].WriteAccess = 1;
        g_CurrentMachine.PageTable->Level3[ Index3 ].ExecuteAccess = 1;
        g_CurrentMachine.PageTable->Level3[ Index3 ].PageFrameNumber = HvGetPhysicalAddress( &g_CurrentMachine.PageTable->Level2[ Index3 ] ) >> 12;
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

            g_CurrentMachine.PageTable->Level2[ Index3 ][ Index2 ].MemoryType = g_EptRangeDescriptors[ CurrentRange ].RegionType;
            g_CurrentMachine.PageTable->Level2[ Index3 ][ Index2 ].ReadAccess = 1;
            g_CurrentMachine.PageTable->Level2[ Index3 ][ Index2 ].WriteAccess = 1;
            g_CurrentMachine.PageTable->Level2[ Index3 ][ Index2 ].ExecuteAccess = 1;
            g_CurrentMachine.PageTable->Level2[ Index3 ][ Index2 ].LargePage = 1;
            g_CurrentMachine.PageTable->Level2[ Index3 ][ Index2 ].PageFrameNumber = (
                g_EptRangeDescriptors[ CurrentRange ].RegionBase +
                MemoryMappingCount ) >> 12;

            //DbgPrint( "Mapping %p %d %d\n", g_EptRangeDescriptors[ CurrentRange ].RegionBase + MemoryMappingCount, Index3, Index2 );

            MemoryMappingCount += 0x200000;
        }

    }

    Ept.Long = 0;
    Ept.PageFrameNumber = HvGetPhysicalAddress( g_CurrentMachine.PageTable->Level4 ) >> 12;
    Ept.PageWalkLength = 3;
    Ept.EnableAccessAndDirtyFlags = FALSE;
    Ept.MemoryType = MEMORY_TYPE_WRITE_BACK;

    g_CurrentMachine.EptPointer = Ept;

    return HVSTATUS_SUCCESS;
}

HVSTATUS
EptTerminateMachine(

)
{
    PLIST_ENTRY Flink;
    PVMX_PAGE_HOOK PageHook;
    PVMX_PAGE_TABLE PageTable;

    ExFreePoolWithTag( g_CurrentMachine.PageTable, EPT_POOL_TAG );

    if ( g_CurrentMachine.HookHead != NULL ) {

        Flink = g_CurrentMachine.HookHead;
        do {
            PageHook = CONTAINING_RECORD( Flink, VMX_PAGE_HOOK, HookLinks );
            Flink = Flink->Flink;
            ExFreePoolWithTag( PageHook, EPT_POOL_TAG );

        } while ( Flink != g_CurrentMachine.HookHead );

        g_CurrentMachine.HookHead = NULL;
    }

    if ( g_CurrentMachine.TableHead != NULL ) {

        Flink = g_CurrentMachine.TableHead;
        do {
            PageTable = CONTAINING_RECORD( Flink, VMX_PAGE_TABLE, TableLinks );
            Flink = Flink->Flink;
            ExFreePoolWithTag( PageTable, EPT_POOL_TAG );

        } while ( Flink != g_CurrentMachine.TableHead );

        g_CurrentMachine.TableHead = NULL;
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

