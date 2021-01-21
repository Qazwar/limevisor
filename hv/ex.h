


#pragma once

EXTERN PVMX_EXIT_HANDLER g_ExitHandlers[ VMX_EXIT_REASON_MAXIMUM ];

HVSTATUS
ExHandleEptMisconfig(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExHandleUnimplemented(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExHandleHardFault(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExHandleCpuid(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExHandleVmx(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExHandleSetbv(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExHandleInvd(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExHandleControlAccess(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExHandleMsrAccess(
    __in PVMX_PROCESSOR_STATE ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
);

HVSTATUS
ExInterruptGuest(
    __in     ULONG32            Vector,
    __in     VMX_INTERRUPT_TYPE Type,
    __in_opt ULONG32            ErrorCode
);

