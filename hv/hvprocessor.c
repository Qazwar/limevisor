


#include "hv.h"

PVMX_PROCESSOR_STATE g_ProcessorState;

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

    if (IdRegisters[ 1 ] != 'uneG' ||
        IdRegisters[ 3 ] != 'Ieni' ||
        IdRegisters[ 2 ] != 'letn') {

        HvTraceBasic( "CPU vendor unsupported.\n" );

        return HVSTATUS_UNSUPPORTED;
    }

    __cpuidex( IdRegisters, 1, 0 );

    //
    //  VMX feature present bit.
    //

    if (( IdRegisters[ 2 ] & ( 1 << 5 ) ) == 0) {

        return HVSTATUS_UNSUPPORTED;
    }

    FeatureControl = __readmsr( IA32_FEATURE_CONTROL );

    //
    //  check the lock bit, if it isn't present, 
    //  set the vmx bit and lock bit.
    //

    if (( FeatureControl & ( 1 << 0 ) ) == 0) {

        FeatureControl |= ( 1 << 0 );   //Lock
        FeatureControl |= ( 1 << 2 );   //VMX
        __writemsr( IA32_FEATURE_CONTROL, FeatureControl );
    }

    //
    //  if the lock bit is set, the bios has locked
    //  some options in place, check if vmx is enabled.
    //

    else if (( FeatureControl & ( 1 << 2 ) ) == 0) {

        return HVSTATUS_UNSUPPORTED;
    }

    VpidCapabilities = __readmsr( IA32_VMX_EPT_VPID_CAP );

    //
    //  check for the support of page walk length of 4,
    //  ept memory type writeback and large PML2s
    //

    if (( VpidCapabilities & ( 1 << 6 ) ) == 0 ||
        ( VpidCapabilities & ( 1 << 16 ) ) == 0 ||
        ( VpidCapabilities & ( 1 << 14 ) ) == 0) {

        return HVSTATUS_UNSUPPORTED;
    }

    MtrrDefType = __readmsr( IA32_MTRR_DEF_TYPE );

    //
    //  check if MTRR's are enabled, we're currently
    //  using these to get a memory map.
    //

    if (( MtrrDefType & ( 1 << 11 ) ) == 0) {

        return HVSTATUS_UNSUPPORTED;
    }

    return HVSTATUS_SUCCESS;
}

HVSTATUS
HvInitializeProcessorStates(

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
    g_ProcessorState = ExAllocatePoolWithTag( NonPagedPoolNx, StatePoolSize, ' UPC' );
    RtlZeroMemory( g_ProcessorState, StatePoolSize );

    for (ULONG32 CurrentProcessor = 0; CurrentProcessor < ProcessorCount; CurrentProcessor++) {

        g_ProcessorState[ CurrentProcessor ].OnRegion = ( ULONG64 )MmAllocateContiguousMemory( 0x2000, HighestPhysical );
        g_ProcessorState[ CurrentProcessor ].OnRegionPhysical = HvGetPhysicalAddress( ( PVOID )g_ProcessorState[ CurrentProcessor ].OnRegion );
        RtlZeroMemory( g_ProcessorState[ CurrentProcessor ].OnRegion, 0x2000 );

        g_ProcessorState[ CurrentProcessor ].ControlRegion = ( ULONG64 )MmAllocateContiguousMemory( 0x2000, HighestPhysical );
        g_ProcessorState[ CurrentProcessor ].ControlRegionPhysical = ( ULONG64 )HvGetPhysicalAddress( ( PVOID )g_ProcessorState[ CurrentProcessor ].ControlRegion );
        RtlZeroMemory( g_ProcessorState[ CurrentProcessor ].ControlRegion, 0x2000 );

        g_ProcessorState[ CurrentProcessor ].BitmapMsr = ( ULONG64 )ExAllocatePoolWithTag( NonPagedPool, 0x1000, ' PMB' );
        g_ProcessorState[ CurrentProcessor ].BitmapMsrPhysical = ( ULONG64 )HvGetPhysicalAddress( ( PVOID )g_ProcessorState[ CurrentProcessor ].BitmapMsr );
        RtlZeroMemory( g_ProcessorState[ CurrentProcessor ].BitmapMsr, 0x1000 );

        g_ProcessorState[ CurrentProcessor ].HostStackSize = 0x2000;
        g_ProcessorState[ CurrentProcessor ].HostStack = ( ULONG64 )ExAllocatePoolWithTag( NonPagedPool, g_ProcessorState[ CurrentProcessor ].HostStackSize, ' KTS' );

    }

    return HVSTATUS_SUCCESS;
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

    HVSTATUS hvStatus;
    PVMX_PROCESSOR_STATE ProcessorState;

    ProcessorState = HvGetProcessorState( );

    hvStatus = VmxInitializeProcessor( ProcessorState );
    if (!HV_SUCCESS( hvStatus )) {

        return hvStatus;
    }

    return HVSTATUS_SUCCESS;
}

HVSTATUS
HvTerminateProcessor(

)
{
    HVSTATUS hvStatus;
    PVMX_PROCESSOR_STATE ProcessorState;

    ProcessorState = HvGetProcessorState( );

    hvStatus = VmxTerminateProcessor( ProcessorState );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
HvInitializeVisor(

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
    if (!HV_SUCCESS( hvStatus )) {

        return hvStatus;
    }

    hvStatus = HvInitializeProcessorStates( );
    if (!HV_SUCCESS( hvStatus )) {

        return hvStatus;
    }

    //
    //  initialize the global exit handlers table, 
    //  any zero entries should be replaced with a
    //  pointer to ExHandleUnimplemented.
    //

    for (ULONG64 ExitHandler = 0; ExitHandler < VMX_EXIT_REASON_MAXIMUM; ExitHandler++) {

        if (g_ExitHandlers[ ExitHandler ] == NULL) {

            g_ExitHandlers[ ExitHandler ] = ExHandleUnimplemented;
        }
    }

    hvStatus = EptInitialize( );
    if (!HV_SUCCESS( hvStatus )) {

        return hvStatus;
    }

    hvStatus = MpProcessorExecute( ( PMP_PROCEDURE )&HvInitializeProcessor, NULL );
    if (!HV_SUCCESS( hvStatus )) {

        MpProcessorExecute( ( PMP_PROCEDURE )&HvTerminateProcessor, NULL );
        return hvStatus;
    }

    return HVSTATUS_SUCCESS;
}

HVSTATUS
HvTerminateVisor(

)
{
    HVSTATUS hvStatus;
    PVMX_PROCESSOR_STATE ProcessorState;
    ULONG32 ProcessorCount;

    //
    //  discard the status, this will always be HVSTATUS_SUCCESS
    //  if there are any errors, it should ignore them and move
    //  onto the next processor.
    //

    hvStatus = MpProcessorExecute( ( PMP_PROCEDURE )HvTerminateProcessor, NULL );

    ProcessorState = HvGetProcessorState( );
    ProcessorCount = KeQueryActiveProcessorCount( 0 );

    for (ULONG32 CurrentProcessor = 0; CurrentProcessor < ProcessorCount; CurrentProcessor++) {

        MmFreeContiguousMemory( ( PVOID )g_ProcessorState[ CurrentProcessor ].OnRegion );
        MmFreeContiguousMemory( ( PVOID )g_ProcessorState[ CurrentProcessor ].ControlRegion );
        MmFreeContiguousMemory( ( PVOID )g_ProcessorState[ CurrentProcessor ].BitmapMsr );
        ExFreePoolWithTag( ( PVOID )g_ProcessorState[ CurrentProcessor ].HostStack, ' KTS' );
    }

    ExFreePoolWithTag( g_ProcessorState, ' UPC' );

    return HVSTATUS_SUCCESS;
}
