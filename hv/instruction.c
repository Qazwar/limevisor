


#include "hv.h"

//
//  a lot of function's are replaced by Exf
//  function's inside the internal vmexit handler.
//  these are faster for single instructions
//

HVSTATUS
ExHandleCpuid(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    int Registers[ 4 ];

    if (( TrapFrame->Rax & 0xFFFFFFFF ) == 0) {
        Registers[ 1 ] = 'aerG';
        Registers[ 3 ] = 'aBes';
        Registers[ 2 ] = '  ll';
        Registers[ 0 ] = 0;
    }
    else if (( TrapFrame->Rax & 0xFFFFFFFF ) == 0xD3ADB00B) {

        TrapFrame->Rax = HVSTATUS_UNSUCCESSFUL;
        return HVSTATUS_UNSUCCESSFUL;
    }
    else if (( TrapFrame->Rax & 0xFFFFFFFF ) == 1) {

        __cpuidex( Registers, ( int )TrapFrame->Rax, ( int )TrapFrame->Rcx );

        TrapFrame->Rcx &= ~5; // vmx bit
    }
    else {

        __cpuidex( Registers, ( int )TrapFrame->Rax, ( int )TrapFrame->Rcx );
    }


    TrapFrame->Rax = Registers[ 0 ];
    TrapFrame->Rbx = Registers[ 1 ];
    TrapFrame->Rcx = Registers[ 2 ];
    TrapFrame->Rdx = Registers[ 3 ];

    return HVSTATUS_SUCCESS;
}

HVSTATUS
ExHandleSetbv(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    _xsetbv( ( unsigned int )TrapFrame->Rcx, ( TrapFrame->Rdx << 32 ) | TrapFrame->Rax );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
ExHandleInvd(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    __wbinvd( );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
ExHandleControlAccess(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    //
    //  function wont actually be called yet, 
    //  because of vm control.
    //

    VMX_EXIT_QUALIFICATION_MOV_CR MovCr;
    PULONG64 Register;

    MovCr.Value = ExitState->ExitQualification;
    Register = &( ( PULONG64 )&TrapFrame->Rax )[ MovCr.Register ];

    switch (MovCr.AccessType) {
    case VMX_EXIT_QUALIFICATION_ACCESS_MOV_TO_CR:
        switch (MovCr.ControlRegister) {
        case 0:
            __vmx_vmwrite( VMCS_GUEST_CR0, *Register );
            __vmx_vmwrite( VMCS_CTRL_CR0_READ_SHADOW, *Register );
            break;
        case 3:
            __vmx_vmwrite( VMCS_GUEST_CR3, *Register );
            break;
        case 4:
            __vmx_vmwrite( VMCS_GUEST_CR4, *Register );
            __vmx_vmwrite( VMCS_CTRL_CR4_GUEST_HOST_MASK, *Register );
            break;
        default:
            HvTraceBasic( "bro what the fuck is a cr%d\n", MovCr.ControlRegister );
            DbgBreakPoint( );
            break;
        }
        break;
    case VMX_EXIT_QUALIFICATION_ACCESS_MOV_FROM_CR:
        switch (MovCr.ControlRegister) {
        case 0:
            __vmx_vmread( VMCS_GUEST_CR0, Register );
            break;
        case 3:
            __vmx_vmread( VMCS_GUEST_CR3, Register );
            break;
        case 4:
            __vmx_vmread( VMCS_GUEST_CR4, Register );
            *Register &= ~0x2000;
            break;
        default:
            HvTraceBasic( "bro what the fuck is a cr%d\n", MovCr.ControlRegister );
            DbgBreakPoint( );
            break;
        }
        break;

        //
        //  not properly implemented.
        //

    default:

        DbgBreakPoint( );
        break;
    }

    return HVSTATUS_SUCCESS;
}

HVSTATUS
ExHandleMsrAccess(
    __in PVMX_PROCESSOR_STATE   ProcessorState,
    __in PVMX_EXIT_TRAP_FRAME   TrapFrame,
    __in PVMX_EXIT_STATE        ExitState
)
{
    ProcessorState;
    TrapFrame;
    ExitState;

    switch (ExitState->ExitReason) {
    case VMX_EXIT_REASON_EXECUTE_RDMSR:;

        ULONG64 Result = __readmsr( ( ULONG32 )TrapFrame->Rcx );

        TrapFrame->Rax = ( ULONG32 )( Result );
        TrapFrame->Rdx = ( ULONG32 )( Result >> 32 );
        break;
    case VMX_EXIT_REASON_EXECUTE_WRMSR:;

        __writemsr( ( ULONG32 )TrapFrame->Rcx, ( TrapFrame->Rdx << 32 ) | TrapFrame->Rcx );
        break;
    }

    return HVSTATUS_SUCCESS;
}
