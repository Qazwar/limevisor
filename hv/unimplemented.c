


#include "hv.h"

HVSTATUS
ExHandleUnimplemented(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    DbgPrint( "ExHandleUnimplemented: %x %p\n", ExitState->ExitReason, ExitState->CurrentIp );

    return HVSTATUS_SUCCESS;
}