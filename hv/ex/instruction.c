


#include "hv.h"

//
//  a lot of function's are replaced by Exf
//  function's inside the internal vmexit handler.
//  these are faster for single instructions
//

HVSTATUS
ExHandleCpuid(
    __in PVMX_PCB             Processor,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    Processor;
    TrapFrame;
    ExitState;

    int IdRegisters[ 4 ];

    if ( ( TrapFrame->Rax & 0xFFFFFFFF ) == 0 ) {
        IdRegisters[ 1 ] = 'aerG';
        IdRegisters[ 3 ] = 'aBes';
        IdRegisters[ 2 ] = '  ll';
        IdRegisters[ 0 ] = 0;
    }
    else if ( ( TrapFrame->Rax & 0xFFFFFFFF ) == 0xD3ADB00B ) {

        TrapFrame->Rax = 0xD3ADB00C;
        return HVSTATUS_UNSUCCESSFUL;
    }
    else if ( ( TrapFrame->Rax & 0xFFFFFFFF ) == 1 ) {

        __cpuidex( IdRegisters, ( int )TrapFrame->Rax, ( int )TrapFrame->Rcx );

        IdRegisters[ 2 ] &= ~( 1 << 5 ); // vmx bit
    }
    else {

        __cpuidex( IdRegisters, ( int )TrapFrame->Rax, ( int )TrapFrame->Rcx );
    }


    TrapFrame->Rax = IdRegisters[ 0 ];
    TrapFrame->Rbx = IdRegisters[ 1 ];
    TrapFrame->Rcx = IdRegisters[ 2 ];
    TrapFrame->Rdx = IdRegisters[ 3 ];

    return HVSTATUS_SUCCESS;
}

HVSTATUS
ExHandleSetbv(
    __in PVMX_PCB             Processor,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    Processor;
    TrapFrame;
    ExitState;

    _xsetbv( ( unsigned int )TrapFrame->Rcx, ( TrapFrame->Rdx << 32 ) | TrapFrame->Rax );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
ExHandleInvd(
    __in PVMX_PCB             Processor,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    Processor;
    TrapFrame;
    ExitState;

    __wbinvd( );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
ExHandleControlAccess(
    __in PVMX_PCB             Processor,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    Processor;
    TrapFrame;
    ExitState;

    //
    //  function wont actually be called yet, 
    //  because of vm control.
    //

    VMX_EQ_ACCESS_CONTROL MovCr;
    PULONG64 Register;

    MovCr.Long = ExitState->ExitQualification;
    Register = &( ( PULONG64 )&TrapFrame->Rax )[ MovCr.Register ];

    switch ( MovCr.AccessType ) {
    case VMX_EXIT_QUALIFICATION_ACCESS_MOV_TO_CR:
        switch ( MovCr.ControlRegister ) {
        case 0:
            __vmx_vmwrite( VMCS_GUEST_CR0, *Register );
            __vmx_vmwrite( VMCS_CTRL_CR0_READ_SHADOW, *Register );
            break;
        case 3:
            __vmx_vmwrite( VMCS_GUEST_CR3, *Register );
            break;
        case 4:
            *Register &= ~0x2000;
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
        switch ( MovCr.ControlRegister ) {
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
    __in PVMX_PCB             Processor,
    __in PVMX_EXIT_TRAP_FRAME TrapFrame,
    __in PVMX_EXIT_STATE      ExitState
)
{
    Processor;
    TrapFrame;
    ExitState;
    ULONG64 Access;

    //HvTraceBasic( "MSR Access: %.8x\n", TrapFrame->Rcx );

    switch ( ExitState->ExitReason ) {
    case VMX_EXIT_REASON_EXECUTE_RDMSR:;

        Access = __readmsr( ( ULONG32 )TrapFrame->Rcx );

        TrapFrame->Rax = ( ULONG32 )( Access ) & ~( 1 << 2 );
        TrapFrame->Rdx = ( ULONG32 )( Access >> 32 );
        break;
    case VMX_EXIT_REASON_EXECUTE_WRMSR:;
        TrapFrame->Rax &= ~( 1 << 2 );
        __writemsr( ( ULONG32 )TrapFrame->Rcx, ( TrapFrame->Rdx << 32 ) | TrapFrame->Rcx );
        break;
    }

    return HVSTATUS_SUCCESS;
}
