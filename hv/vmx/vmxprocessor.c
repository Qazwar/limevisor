


#include "hv.h"

HVSTATUS
VmxInitializeProcessor(
    __in PVMX_PCB Processor
)
{
    //
    //  setup vmx on each logical processor
    //
    //  irql = dispatch_level
    //

    HVSTATUS hvStatus;
    UCHAR CarryFlag;

    __writecr4( __readcr4( ) | 0x2000 );

    __writecr0( ( __readcr0( ) | __readmsr( IA32_VMX_CR0_FIXED0 ) ) & __readmsr( IA32_VMX_CR0_FIXED1 ) );
    __writecr4( ( __readcr4( ) | __readmsr( IA32_VMX_CR4_FIXED0 ) ) & __readmsr( IA32_VMX_CR4_FIXED1 ) );

    *( ULONG32* )Processor->OnRegion = ( ULONG32 )__readmsr( IA32_VMX_BASIC );
    *( ULONG32* )Processor->ControlRegion = ( ULONG32 )__readmsr( IA32_VMX_BASIC );

    CarryFlag = __vmx_on( &Processor->OnRegionPhysical );
    if ( CarryFlag ) {

        __writecr4( __readcr4( ) & ~0x2000 );
        return HVSTATUS_UNSUCCESSFUL;
    }

    CarryFlag = __vmx_vmclear( &Processor->ControlRegionPhysical );
    if ( CarryFlag ) {

        __writecr4( __readcr4( ) & ~0x2000 );
        return HVSTATUS_UNSUCCESSFUL;
    }

    CarryFlag = __vmx_vmptrld( &Processor->ControlRegionPhysical );
    if ( CarryFlag ) {

        __writecr4( __readcr4( ) & ~0x2000 );
        return HVSTATUS_UNSUCCESSFUL;
    }

    VmxInitializeProcessorControl( Processor );

    hvStatus = VmxLaunchAndStore( );
    if ( !HV_SUCCESS( hvStatus ) ) {

        HvTraceBasic( "VMX launch failure.\n" );
        __writecr4( __readcr4( ) & ~0x2000 );
        return hvStatus;
    }

    HvTraceBasic( "VMX initialized on processor%d\n", HvGetCurrentProcessorNumber( ) );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
VmxTerminateProcessor(
    __in PVMX_PCB Processor
)
{
    Processor;
    int IdRegisters[ 4 ];

    __cpuidex( IdRegisters, 0xD3ADB00B, 0 );

    if ( IdRegisters[ 0 ] != 0xD3ADB00C ) {

        KeBugCheck( HYPERVISOR_ERROR );
    }

    __writecr4( __readcr4( ) & ~0x2000 );

    HvTraceBasic( "VMX terminated on processor%d\n", KeGetCurrentProcessorNumber( ) );

    return HVSTATUS_SUCCESS;
}