


#include "hv.h"

HVSTATUS
ExHandleUnimplemented(
    __in PVMX_PCB             Processor,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    Processor;
    TrapFrame;
    ExitState;

    DbgPrint( "ExHandleUnimplemented: %x %p\n", ExitState->ExitReason, ExitState->CurrentIp );

    return HVSTATUS_SUCCESS;
}