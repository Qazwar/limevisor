


#include "hv.h"

HV_VMM g_CurrentMachine;

HVSTATUS
HvCheckProcessorSupport(

)
{
    ULONG64 FeatureControl;
    ULONG64 VpidCapabilities;
    ULONG64 MtrrDefType;

    int IdRegisters[ 4 ];

    __cpuidex( IdRegisters, 0, 0 );

    //
    //  we only support intel vt-x
    //

    if ( IdRegisters[ 1 ] != 'uneG' ||
         IdRegisters[ 3 ] != 'Ieni' ||
         IdRegisters[ 2 ] != 'letn' ) {

        HvTraceBasic( "CPU vendor unsupported.\n" );

        return HVSTATUS_UNSUPPORTED;
    }

    __cpuidex( IdRegisters, 1, 0 );

    //
    //  VMX feature present bit.
    //

    if ( ( IdRegisters[ 2 ] & ( 1 << 5 ) ) == 0 ) {

        return HVSTATUS_UNSUPPORTED;
    }

    FeatureControl = __readmsr( IA32_FEATURE_CONTROL );

    //
    //  check the lock bit, if it isn't present, 
    //  set the vmx bit and lock bit.
    //

    if ( ( FeatureControl & ( 1 << 0 ) ) == 0 ) {

        FeatureControl |= ( 1 << 0 );   //Lock
        FeatureControl |= ( 1 << 2 );   //VMX
        __writemsr( IA32_FEATURE_CONTROL, FeatureControl );
    }

    //
    //  if the lock bit is set, the bios has locked
    //  some options in place, check if vmx is enabled.
    //

    else if ( ( FeatureControl & ( 1 << 2 ) ) == 0 ) {

        return HVSTATUS_UNSUPPORTED;
    }

    VpidCapabilities = __readmsr( IA32_VMX_EPT_VPID_CAP );

    //
    //  check for the support of page walk length of 4,
    //  ept memory type writeback and large PML2s
    //

    if ( ( VpidCapabilities & ( 1 << 6 ) ) == 0 ||
        ( VpidCapabilities & ( 1 << 16 ) ) == 0 ||
         ( VpidCapabilities & ( 1 << 14 ) ) == 0 ) {

        return HVSTATUS_UNSUPPORTED;
    }

    MtrrDefType = __readmsr( IA32_MTRR_DEF_TYPE );

    //
    //  check if MTRR's are enabled, we're currently
    //  using these to get a memory map.
    //

    if ( ( MtrrDefType & ( 1 << 11 ) ) == 0 ) {

        return HVSTATUS_UNSUPPORTED;
    }

    return HVSTATUS_SUCCESS;
}

VOID
HvAllocateMachine(

)
{

    //
    //  this function is responsible for allocating g_ProcessorState
    //  and setting up each processor's state. it should allocate the
    //  on region, control region, host exit stack and the msr bitmap
    //  for the control structure.
    //

    ULONG32 StatePoolSize;
    ULONG32 ProcessorCount;

    PHYSICAL_ADDRESS HighestPhysical = { ~0ULL };

    ProcessorCount = KeQueryActiveProcessorCount( 0 );
    StatePoolSize = sizeof( VMX_PROCESSOR_STATE ) * ProcessorCount;
    g_CurrentMachine.ProcessorState = ExAllocatePoolWithTag( NonPagedPoolNx, StatePoolSize, HV_POOL_TAG );
    RtlZeroMemory( g_CurrentMachine.ProcessorState, StatePoolSize );

    for ( ULONG32 CurrentProcessor = 0; CurrentProcessor < ProcessorCount; CurrentProcessor++ ) {

        g_CurrentMachine.ProcessorState[ CurrentProcessor ].OnRegion = ( ULONG64 )MmAllocateContiguousMemory( 0x2000, HighestPhysical );
        g_CurrentMachine.ProcessorState[ CurrentProcessor ].OnRegionPhysical =
            HvGetPhysicalAddress( ( PVOID )g_CurrentMachine.ProcessorState[ CurrentProcessor ].OnRegion );
        RtlZeroMemory( g_CurrentMachine.ProcessorState[ CurrentProcessor ].OnRegion, 0x2000 );

        g_CurrentMachine.ProcessorState[ CurrentProcessor ].ControlRegion = ( ULONG64 )MmAllocateContiguousMemory( 0x2000, HighestPhysical );
        g_CurrentMachine.ProcessorState[ CurrentProcessor ].ControlRegionPhysical =
            HvGetPhysicalAddress( ( PVOID )g_CurrentMachine.ProcessorState[ CurrentProcessor ].ControlRegion );
        RtlZeroMemory( g_CurrentMachine.ProcessorState[ CurrentProcessor ].ControlRegion, 0x2000 );

        g_CurrentMachine.ProcessorState[ CurrentProcessor ].HostStackSize = 0x2000;
        g_CurrentMachine.ProcessorState[ CurrentProcessor ].HostStack =
            ( ULONG64 )ExAllocatePoolWithTag( NonPagedPool, g_CurrentMachine.ProcessorState[ CurrentProcessor ].HostStackSize, HV_POOL_TAG );
    }

    /*
        • Read bitmap for low MSRs (located at the MSR-bitmap address). This contains one bit for each MSR address
            in the range 00000000H to 00001FFFH. The bit determines whether an execution of RDMSR applied to that
            MSR causes a VM exit.
        • Read bitmap for high MSRs (located at the MSR-bitmap address plus 1024). This contains one bit for each
            MSR address in the range C0000000H toC0001FFFH. The bit determines whether an execution of RDMSR
            applied to that MSR causes a VM exit.
        • Write bitmap for low MSRs (located at the MSR-bitmap address plus 2048). This contains one bit for each
            MSR address in the range 00000000H to 00001FFFH. The bit determines whether an execution of WRMSR
            applied to that MSR causes a VM exit.
        • Write bitmap for high MSRs (located at the MSR-bitmap address plus 3072). This contains one bit for each
            MSR address in the range C0000000H toC0001FFFH. The bit determines whether an execution of WRMSR
            applied to that MSR causes a VM exit.
    */

    g_CurrentMachine.MsrMap = ( ULONG64 )ExAllocatePoolWithTag( NonPagedPool, 0x1000, HV_POOL_TAG );
    g_CurrentMachine.MsrMapPhysical = ( ULONG64 )HvGetPhysicalAddress( ( PVOID )g_CurrentMachine.MsrMap );
    RtlZeroMemory( g_CurrentMachine.MsrMap, 0x1000 );

    *( ULONG64* )( g_CurrentMachine.MsrMap + ( IA32_FEATURE_CONTROL / 64 ) ) |= 1ULL << ( IA32_FEATURE_CONTROL % 64 );
    *( ULONG64* )( g_CurrentMachine.MsrMap + ( IA32_FEATURE_CONTROL / 64 ) + 2048 ) |= 1ULL << ( IA32_FEATURE_CONTROL % 64 );
}

VOID
HvFreeMachine(

)
{
    ULONG32 ProcessorCount;

    ProcessorCount = KeQueryActiveProcessorCount( 0 );

    for ( ULONG32 CurrentProcessor = 0; CurrentProcessor < ProcessorCount; CurrentProcessor++ ) {

        MmFreeContiguousMemory( ( PVOID )g_CurrentMachine.ProcessorState[ CurrentProcessor ].OnRegion );
        MmFreeContiguousMemory( ( PVOID )g_CurrentMachine.ProcessorState[ CurrentProcessor ].ControlRegion );
        ExFreePoolWithTag( ( PVOID )g_CurrentMachine.ProcessorState[ CurrentProcessor ].HostStack, HV_POOL_TAG );
    }

    ExFreePoolWithTag( g_CurrentMachine.ProcessorState, HV_POOL_TAG );

    ExFreePoolWithTag( ( PVOID )g_CurrentMachine.MsrMap, HV_POOL_TAG );
}

HVSTATUS
HvInitializeProcessor(

)
{
    //
    //  responsible for initializing processor-specific
    //  control. this will be executed on each processor.
    //
    //  irql = dispatch_level
    //

    return VmxInitializeProcessor( &g_CurrentMachine.ProcessorState[ KeGetCurrentProcessorNumber( ) ] );
}

HVSTATUS
HvTerminateProcessor(

)
{

    return VmxTerminateProcessor( &g_CurrentMachine.ProcessorState[ HvGetCurrentProcessorNumber( ) ] );
}

HVSTATUS
HvInitializeMachine(

)
{
    //
    //  this function is responsible for setting up
    //  and entering each processor into a virtualized
    //  state, it should also initialize any global vars
    //  and other important things.
    //

    HVSTATUS hvStatus;

    hvStatus = HvCheckProcessorSupport( );
    if ( !HV_SUCCESS( hvStatus ) ) {

        return hvStatus;
    }

    HvAllocateMachine( );

    //
    //  initialize the global exit handlers table, 
    //  any zero entries should be replaced with a
    //  pointer to ExHandleUnimplemented.
    //

    for ( ULONG64 ExitHandler = 0; ExitHandler < VMX_EXIT_REASON_MAXIMUM; ExitHandler++ ) {

        if ( g_ExitHandlers[ ExitHandler ] == NULL ) {

            g_ExitHandlers[ ExitHandler ] = ExHandleUnimplemented;
        }
    }

    hvStatus = EptInitialize( );
    if ( !HV_SUCCESS( hvStatus ) ) {

        HvFreeMachine( );
        return hvStatus;
    }

    hvStatus = EptInitializeMachine( );
    if ( !HV_SUCCESS( hvStatus ) ) {

        HvFreeMachine( );
        return hvStatus;
    }

    hvStatus = MpProcessorExecute( ( PMP_PROCEDURE )HvInitializeProcessor, NULL );
    if ( !HV_SUCCESS( hvStatus ) ) {

        HvFreeMachine( );
        //MpProcessorExecute( ( PMP_PROCEDURE )HvTerminateProcessor, NULL );
        return hvStatus;
    }

    return HVSTATUS_SUCCESS;
}

HVSTATUS
HvTerminateMachine(

)
{
    HVSTATUS hvStatus;

    //
    //  discard the status, this will always be HVSTATUS_SUCCESS
    //  if there are any errors, it should ignore them and move
    //  onto the next processor.
    //

    hvStatus = MpProcessorExecute( ( PMP_PROCEDURE )HvTerminateProcessor, NULL );

    HvFreeMachine( );

    return hvStatus;
}
