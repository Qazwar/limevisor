


#include "hv.h"

HVSTATUS
ExHandleEptMisconfig(
    __in PVMX_PCB             Processor,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    Processor;
    TrapFrame;
    ExitState;

    HvTraceBasic( "EPT misconfig, terminating processor%d\n", HvGetCurrentProcessorNumber( ) );
    DbgBreakPoint( );

    return HVSTATUS_UNSUCCESSFUL;
}

HVSTATUS
ExHandleHardFault(
    __in PVMX_PCB ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
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