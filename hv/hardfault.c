


#include "hv.h"

HVSTATUS
ExHandleEptMisconfig(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    HvTraceBasic( "EPT misconfig, terminating processor%d\n", KeGetCurrentProcessorNumber( ) );
    DbgBreakPoint( );

    return HVSTATUS_UNSUCCESSFUL;
}

HVSTATUS
ExHandleHardFault(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    //
    //  we're kinda fucked if this is ever executed
    //

    DbgBreakPoint( );

    return HVSTATUS_SUCCESS;
}