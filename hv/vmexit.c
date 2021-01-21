


#include "hv.h"

#define VMX_DECLARE_HANDLER( _, __ ) [ _ ] = ( PVMX_EXIT_HANDLER )( __ ),

PVMX_EXIT_HANDLER g_ExitHandlers[ VMX_EXIT_REASON_MAXIMUM ] = {

    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_TRIPLE_FAULT, ExHandleHardFault )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_NMI_WINDOW, ExHandleHardFault )


    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EPT_VIOLATION, EptViolationFault )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_CPUID, ExHandleCpuid )

    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_MOV_CR, ExHandleControlAccess )

    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_RDMSR, ExHandleMsrAccess )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_WRMSR, ExHandleMsrAccess )

    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_INVD, ExHandleInvd )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_XSETBV, ExHandleSetbv )

    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMXON, ExHandleVmx )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMXOFF, ExHandleVmx )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMCLEAR, ExHandleVmx )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMPTRLD, ExHandleVmx )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMPTRST, ExHandleVmx )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMREAD, ExHandleVmx )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMWRITE, ExHandleVmx )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMLAUNCH, ExHandleVmx )
    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EXECUTE_VMRESUME, ExHandleVmx )

    VMX_DECLARE_HANDLER( VMX_EXIT_REASON_EPT_MISCONFIGURATION, ExHandleEptMisconfig )


};

HVSTATUS
VmxHandleExit(
    __in PVMX_EXIT_TRAP_FRAME TrapFrame
)
{
    VMX_EXIT_STATE ExitState;
    PVMX_PROCESSOR_STATE ProcessorState;

    ULONG64 ExitInstructionLength;

    ExitState.FinalStatus = HVSTATUS_SUCCESS;
    ExitState.IncrementIp = TRUE;

    ProcessorState = HvGetProcessorState( HvGetCurrentProcessorNumber( ) );

    __vmx_vmread( VMCS_EXIT_REASON, &ExitState.ExitReason );
    __vmx_vmread( VMCS_EXIT_QUALIFICATION, &ExitState.ExitQualification );

    __vmx_vmread( VMCS_GUEST_RSP, &TrapFrame->Rsp );
    __vmx_vmread( VMCS_GUEST_RIP, &ExitState.CurrentIp );

    ExitState.ExitReason &= 0xFFFF;

    if ( ExitState.ExitReason < VMX_EXIT_REASON_MAXIMUM ) {

        //HvTraceBasic( "exit: %d\n", ExitState.ExitReason );
        //ExInterruptGuest( 3, VmxInterruptHardwareException, 0 );
        ExitState.FinalStatus = g_ExitHandlers[ ExitState.ExitReason ]( ProcessorState, TrapFrame, &ExitState );
    }

    if ( !HV_SUCCESS( ExitState.FinalStatus ) ) {

        HvTraceBasic( "Reason: %#.8x caused vmxoff.\n", ExitState.ExitReason );
    }

    if ( ExitState.IncrementIp ) {

        __vmx_vmread( VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstructionLength );
        ExitState.CurrentIp += ExitInstructionLength;
    }

    __vmx_vmwrite( VMCS_GUEST_RIP, ExitState.CurrentIp );
    __vmx_vmwrite( VMCS_GUEST_RSP, TrapFrame->Rsp );

    return ExitState.FinalStatus;
}