


#include "hv.h"

HVSTATUS
EptViolationFault(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    HVSTATUS hvStatus;

    ULONG64 AddressAccessed;
    VMX_EQ_EPT_VIOLATION Qualification;

    __vmx_vmread( VMCS_GUEST_PHYSICAL_ADDRESS, &AddressAccessed );
    Qualification.Long = ExitState->ExitQualification;

    HvTraceBasic( "EPT violation %p\n", AddressAccessed );

    ExitState->IncrementIp = FALSE;

    if ( !Qualification.CausedByTranslation ) {

        HvTraceBasic( "#VE not caused by translation.\n" );
        ExInterruptGuest( 0xd, VmxInterruptHardwareException, 0 );
        return HVSTATUS_SUCCESS;
    }

    hvStatus = EptPageHookFault( ProcessorState, TrapFrame, ExitState, AddressAccessed );
    if ( HV_SUCCESS( hvStatus ) ) {

        return hvStatus;
    }

    return HVSTATUS_UNSUCCESSFUL;
}