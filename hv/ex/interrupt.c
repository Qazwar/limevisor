


#include "hv.h"

HVSTATUS
ExInterruptGuest(
    __in     ULONG32            Vector,
    __in     VMX_INTERRUPT_TYPE Type,
    __in_opt ULONG32            ErrorCode
)
{
    //
    // Send an interrupt to the guest, ErrorCode is only
    // used if it's a ExInterruptHardwareException or
    // ExInterruptSoftwareException, and the vector is valid
    //
    // irql >= passive_level
    //

    VMX_INTERRUPT_INFORMATION Interrupt;

    Interrupt.Long = 0;
    Interrupt.Valid = 1;
    Interrupt.Type = Type;
    Interrupt.Vector = Vector;

    if ( Type == VmxInterruptHardwareException ||
         Type == VmxInterruptSoftwareException ) {
        switch ( Vector ) {
        case 8:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 17:
            Interrupt.ErrorCode = 1;
            __vmx_vmwrite( VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, Interrupt.Long );
            __vmx_vmwrite( VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode );
            break;
        default:
            Interrupt.ErrorCode = 0;
            __vmx_vmwrite( VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, Interrupt.Long );
            break;
        }
    }

    return HVSTATUS_SUCCESS;
}