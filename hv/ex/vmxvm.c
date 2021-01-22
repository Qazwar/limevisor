


#include "hv.h"

HVSTATUS
ExHandleVmx(
    __in PVMX_PCB             Processor,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    Processor;
    TrapFrame;
    ExitState;

    return ExInterruptGuest( 0x6, VmxInterruptHardwareException, 0 );
}
