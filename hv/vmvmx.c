


#include "hv.h"

HVSTATUS
ExHandleVmx(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    ULONG64 EFlags;

    __vmx_vmread( VMCS_GUEST_RFLAGS, &EFlags );
    __vmx_vmwrite( VMCS_GUEST_RFLAGS, EFlags | 0x1 );

    return HVSTATUS_SUCCESS;
}
