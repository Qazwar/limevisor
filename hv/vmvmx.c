


#include "hv.h"

HVSTATUS
ExHandleVmx(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    return ExInterruptGuest( 0x6, VmxInterruptHardwareException, 0 );
}
