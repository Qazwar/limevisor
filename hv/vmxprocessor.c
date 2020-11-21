


#include "hv.h"

HVSTATUS
VmxInitializeProcessor(
    __in PVMX_PROCESSOR_STATE ProcessorState
)
{
    //
    //  setup vmx on each logical processor
    //
    //  irql = dispatch_level
    //

    HVSTATUS hvStatus;
    UCHAR CarryFlag;

    EptInitializeProcessor( ProcessorState );

    static ULONG64 PageHooking = 0;
    static ULONG64 PageHook = 0;

    if (PageHooking == 1) {
        DbgBreakPoint( );

        PageHooking = ( ULONG64 )ExAllocatePoolWithTag( NonPagedPool, 0x1000, ' GOP' );
        PageHook = ( ULONG64 )ExAllocatePoolWithTag( NonPagedPool, 0x1000, ' GOP' );
        __stosd( ( void* )PageHooking, 'EMIL', 0x1000 / 4 );
        __stosd( ( void* )PageHook, ' GOP', 0x1000 / 4 );

        EptInstallPageHook(
            ProcessorState,
            PageHooking,
            PageHook,
            1,
            EPT_HOOK_BEHAVIOUR_WRITE );

        HvTraceBasic( "Hooking %p, Hook %p\n", PageHooking, PageHook );
    }

    __writecr4( __readcr4( ) | 0x2000 );

    __writecr0( ( __readcr0( ) | __readmsr( IA32_VMX_CR0_FIXED0 ) ) & __readmsr( IA32_VMX_CR0_FIXED1 ) );
    __writecr4( ( __readcr4( ) | __readmsr( IA32_VMX_CR4_FIXED0 ) ) & __readmsr( IA32_VMX_CR4_FIXED1 ) );

    *( ULONG32* )ProcessorState->OnRegion = ( ULONG32 )__readmsr( IA32_VMX_BASIC );
    *( ULONG32* )ProcessorState->ControlRegion = ( ULONG32 )__readmsr( IA32_VMX_BASIC );

    CarryFlag = __vmx_on( &ProcessorState->OnRegionPhysical );
    if (CarryFlag) {

        return HVSTATUS_UNSUCCESSFUL;
    }

    CarryFlag = __vmx_vmclear( &ProcessorState->ControlRegionPhysical );
    if (CarryFlag) {

        return HVSTATUS_UNSUCCESSFUL;
    }

    CarryFlag = __vmx_vmptrld( &ProcessorState->ControlRegionPhysical );
    if (CarryFlag) {

        return HVSTATUS_UNSUCCESSFUL;
    }

    VmxInitializeProcessorControl( ProcessorState );

    hvStatus = VmxLaunchAndStore( );
    if (!HV_SUCCESS( hvStatus )) {

        HvTraceBasic( "VMX Launch failure.\n" );
        return hvStatus;
    }

    HvTraceBasic( "VMX Initialized on processor%d\n", KeGetCurrentProcessorNumber( ) );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
VmxTerminateProcessor(
    __in PVMX_PROCESSOR_STATE ProcessorState
)
{
    HVSTATUS hvStatus;
    int IdRegisters[ 4 ];

    __cpuidex( IdRegisters, 0xD3ADB00B, 0 );

    __writecr4( __readcr4( ) & ~0x2000 );

    hvStatus = EptTerminateProcessor( ProcessorState );

    HvTraceBasic( "VMX Terminated on processor%d\n", KeGetCurrentProcessorNumber( ) );

    return HVSTATUS_SUCCESS;
}