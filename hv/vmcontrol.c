


#include "hv.h"

VOID
VmxCopyControlDescriptor(
    __in ULONG64                        SegmentSelector,
    __in PSEGMENT_DESCRIPTOR_REGISTER   DescriptorTable,
    __in PVMX_SEGMENT_DESCRIPTOR        Descriptor
)
{
    //
    //  copies parts of the GDT segment provided
    //  into a structure that is useful to setting up
    //  vm control.
    //

    ULONG64 Selector;

    PSEGMENT_DESCRIPTOR SegmentDescriptor;

    RtlZeroMemory( Descriptor, sizeof( VMX_SEGMENT_DESCRIPTOR ) );

    //
    //  ignore the rpl, and mark any ldt entries as unusable.
    //

    Selector = SegmentSelector & 0xF8;

    if (Selector == 0 || ( Selector & 4 ) != 0) {

        Descriptor->AccessRights.Unusable = 1;

        return;
    }

    SegmentDescriptor = ( PSEGMENT_DESCRIPTOR )( ( PCHAR )DescriptorTable->BaseAddress + Selector );

    Descriptor->SegmentBase =
        ( SegmentDescriptor->BaseAddressHigh << 24 ) |
        ( SegmentDescriptor->BaseAddressMiddle << 16 ) |
        ( SegmentDescriptor->BaseAddressLow );
    Descriptor->SegmentBase &= 0xFFFFFFFF;

    Descriptor->AccessRights.Value = 0;
    Descriptor->AccessRights.Type = SegmentDescriptor->Type;
    Descriptor->AccessRights.DescriptorType = SegmentDescriptor->DescriptorType;
    Descriptor->AccessRights.DescriptorPrivilegeLevel = SegmentDescriptor->DescriptorPrivilegeLevel;
    Descriptor->AccessRights.Present = SegmentDescriptor->Present;
    Descriptor->AccessRights.Available = SegmentDescriptor->System;
    Descriptor->AccessRights.LongMode = SegmentDescriptor->LongMode;
    Descriptor->AccessRights.DefaultBig = SegmentDescriptor->DefaultBig;
    Descriptor->AccessRights.Granularity = SegmentDescriptor->Granularity;
    Descriptor->AccessRights.Unusable = 0;

    //
    //  the limit is the access byte for TSS's
    //

    Descriptor->SegmentLimit = ( ULONG64 )__segmentlimit( ( ULONG )Selector );

    //
    //  if it's a system segment, it is something like a TSS
    //  where hte next gdt entry occupies another 32 bit base
    //  address value.
    //

    if (SegmentDescriptor->DescriptorType == 0) {

        Descriptor->SegmentBase |= ( ( ULONG64 )SegmentDescriptor->BaseAddressUpper << 32 );
    }

    return;
}

HVSTATUS
VmxInitializeProcessorGuestControl(
    __in PVMX_PROCESSOR_STATE       ProcessorState,
    __in PVMX_PROCESSOR_DESCRIPTOR  ProcessorDescriptor
)
{
    ProcessorState;
    ProcessorDescriptor;

    VMX_SEGMENT_DESCRIPTOR Descriptor;

    __vmx_vmwrite( VMCS_GUEST_VMCS_LINK_POINTER, ( unsigned __int64 )~0i64 );

    __vmx_vmwrite( VMCS_GUEST_DEBUGCTL, __readmsr( IA32_DEBUGCTL ) );

    __vmx_vmwrite( VMCS_GUEST_ACTIVITY_STATE, 0 );
    __vmx_vmwrite( VMCS_GUEST_INTERRUPTIBILITY_STATE, 0 );
    __vmx_vmwrite( VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0 );

#define INIT_GUEST_SEG( SEL, SEG ) \
    VmxCopyControlDescriptor( SEL, &ProcessorDescriptor->GdtRegister, &Descriptor );\
    __vmx_vmwrite( VMCS_GUEST_##SEG##_SELECTOR,         SEL & 0xFF );\
    __vmx_vmwrite( VMCS_GUEST_##SEG##_BASE,             Descriptor.SegmentBase );\
    __vmx_vmwrite( VMCS_GUEST_##SEG##_LIMIT,            Descriptor.SegmentLimit );\
    __vmx_vmwrite( VMCS_GUEST_##SEG##_ACCESS_RIGHTS,    Descriptor.AccessRights.Value );

    INIT_GUEST_SEG( ProcessorDescriptor->SegES, ES );
    INIT_GUEST_SEG( ProcessorDescriptor->SegFS, FS );
    INIT_GUEST_SEG( ProcessorDescriptor->SegGS, GS );
    INIT_GUEST_SEG( ProcessorDescriptor->SegDS, DS );
    INIT_GUEST_SEG( ProcessorDescriptor->SegCS, CS );
    INIT_GUEST_SEG( ProcessorDescriptor->SegSS, SS );
    INIT_GUEST_SEG( ProcessorDescriptor->LdtRegister, LDTR );
    INIT_GUEST_SEG( ProcessorDescriptor->TaskRegister, TR );

#undef INIT_GUEST_SEG

    __vmx_vmwrite( VMCS_GUEST_FS_BASE, __readmsr( IA32_FS_BASE ) );
    __vmx_vmwrite( VMCS_GUEST_GS_BASE, __readmsr( IA32_GS_BASE ) );

    __vmx_vmwrite( VMCS_GUEST_DR7, __readdr( 7 ) );

    __vmx_vmwrite( VMCS_GUEST_CR0, __readcr0( ) );
    __vmx_vmwrite( VMCS_GUEST_CR3, __readcr3( ) );
    __vmx_vmwrite( VMCS_GUEST_CR4, __readcr4( ) );

    __vmx_vmwrite( VMCS_GUEST_GDTR_BASE, ProcessorDescriptor->GdtRegister.BaseAddress );
    __vmx_vmwrite( VMCS_GUEST_GDTR_LIMIT, ProcessorDescriptor->GdtRegister.Limit );

    __vmx_vmwrite( VMCS_GUEST_IDTR_BASE, ProcessorDescriptor->IdtRegister.BaseAddress );
    __vmx_vmwrite( VMCS_GUEST_IDTR_LIMIT, ProcessorDescriptor->IdtRegister.Limit );

    __vmx_vmwrite( VMCS_GUEST_RFLAGS, __readeflags( ) );

    __vmx_vmwrite( VMCS_GUEST_RIP, ( ULONG64 )VmxLaunchAndRestore );

    //
    //  the stack is written inside VmxLaunchVmAndRestore
    //

    __vmx_vmwrite( VMCS_GUEST_RSP, 0 );

    __vmx_vmwrite( VMCS_GUEST_SYSENTER_CS, __readmsr( IA32_SYSENTER_CS ) );
    __vmx_vmwrite( VMCS_GUEST_SYSENTER_EIP, __readmsr( IA32_SYSENTER_EIP ) );
    __vmx_vmwrite( VMCS_GUEST_SYSENTER_ESP, __readmsr( IA32_SYSENTER_ESP ) );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
VmxInitializeProcessorHostControl(
    __in PVMX_PROCESSOR_STATE       ProcessorState,
    __in PVMX_PROCESSOR_DESCRIPTOR  ProcessorDescriptor
)
{
    VMX_SEGMENT_DESCRIPTOR Descriptor;

    __vmx_vmwrite( VMCS_HOST_CS_SELECTOR, ProcessorDescriptor->SegCS & 0xF8 );
    __vmx_vmwrite( VMCS_HOST_DS_SELECTOR, ProcessorDescriptor->SegDS & 0xF8 );
    __vmx_vmwrite( VMCS_HOST_ES_SELECTOR, ProcessorDescriptor->SegES & 0xF8 );
    __vmx_vmwrite( VMCS_HOST_FS_SELECTOR, ProcessorDescriptor->SegFS & 0xF8 );
    __vmx_vmwrite( VMCS_HOST_GS_SELECTOR, ProcessorDescriptor->SegGS & 0xF8 );
    __vmx_vmwrite( VMCS_HOST_TR_SELECTOR, ProcessorDescriptor->TaskRegister & 0xF8 );

    __vmx_vmwrite( VMCS_HOST_IDTR_BASE, ProcessorDescriptor->IdtRegister.BaseAddress );
    __vmx_vmwrite( VMCS_HOST_GDTR_BASE, ProcessorDescriptor->GdtRegister.BaseAddress );

    __vmx_vmwrite( VMCS_HOST_RSP, ProcessorState->HostStack + ProcessorState->HostStackSize );
    __vmx_vmwrite( VMCS_HOST_RIP, ( ULONG64 )VmxHandleExitInternal );

    __vmx_vmwrite( VMCS_HOST_CR0, __readcr0( ) );
    __vmx_vmwrite( VMCS_HOST_CR3, __readcr3( ) );
    __vmx_vmwrite( VMCS_HOST_CR4, __readcr4( ) );

    __vmx_vmwrite( VMCS_HOST_SYSENTER_CS, __readmsr( IA32_SYSENTER_CS ) );
    __vmx_vmwrite( VMCS_HOST_SYSENTER_EIP, __readmsr( IA32_SYSENTER_EIP ) );
    __vmx_vmwrite( VMCS_HOST_SYSENTER_ESP, __readmsr( IA32_SYSENTER_ESP ) );

    VmxCopyControlDescriptor( ProcessorDescriptor->TaskRegister, &ProcessorDescriptor->GdtRegister, &Descriptor );
    __vmx_vmwrite( VMCS_HOST_TR_BASE, Descriptor.SegmentBase );

    __vmx_vmwrite( VMCS_HOST_FS_BASE, __readmsr( IA32_FS_BASE ) );
    __vmx_vmwrite( VMCS_HOST_GS_BASE, __readmsr( IA32_GS_BASE ) );

    return HVSTATUS_SUCCESS;
}

HVSTATUS
VmxInitializeProcessorControl(
    __in PVMX_PROCESSOR_STATE ProcessorState
)
{
    VMX_PROCESSOR_DESCRIPTOR ProcessorDescriptor;

    VmxGetProcessorDescriptor( &ProcessorDescriptor );

    __vmx_vmwrite( VMCS_CTRL_PAGEFAULT_ERROR_CODE_MASK, 0 );
    __vmx_vmwrite( VMCS_CTRL_PAGEFAULT_ERROR_CODE_MATCH, 0 );

    __vmx_vmwrite( VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT, 0 );
    __vmx_vmwrite( VMCS_CTRL_VMEXIT_MSR_STORE_COUNT, 0 );

    __vmx_vmwrite( VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT, 0 );
    __vmx_vmwrite( VMCS_CTRL_VMEXIT_MSR_STORE_COUNT, 0 );

    __vmx_vmwrite( VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 0 );
    __vmx_vmwrite( VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, 0 );

    __vmx_vmwrite( VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, HvAdjustBitControl(
        CPU_BASED_ACTIVATE_MSR_BITMAP |
        CPU_BASED_ACTIVATE_SECONDARY_CONTROLS,
        IA32_VMX_TRUE_PROCBASED_CTLS ) );

    __vmx_vmwrite( VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, HvAdjustBitControl(
        SECONDARY_EXEC_ENABLE_RDTSCP |
        SECONDARY_EXEC_ENABLE_INVPCID |
        SECONDARY_EXEC_XSAVES |
        SECONDARY_EXEC_ENABLE_EPT,
        IA32_VMX_PROCBASED_CTLS2 ) );

    __vmx_vmwrite( VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, HvAdjustBitControl( 0, IA32_VMX_TRUE_PINBASED_CTLS ) );

    __vmx_vmwrite( VMCS_CTRL_CR3_TARGET_COUNT, 0 );
    __vmx_vmwrite( VMCS_CTRL_CR3_TARGET_VALUE_0, 0 );
    __vmx_vmwrite( VMCS_CTRL_CR3_TARGET_VALUE_1, 0 );
    __vmx_vmwrite( VMCS_CTRL_CR3_TARGET_VALUE_2, 0 );
    __vmx_vmwrite( VMCS_CTRL_CR3_TARGET_VALUE_3, 0 );

    __vmx_vmwrite( VMCS_CTRL_CR0_GUEST_HOST_MASK, 0 );
    __vmx_vmwrite( VMCS_CTRL_CR4_GUEST_HOST_MASK, 0 );
    __vmx_vmwrite( VMCS_CTRL_CR0_READ_SHADOW, __readcr0( ) );
    __vmx_vmwrite( VMCS_CTRL_CR4_READ_SHADOW, __readcr4( ) );

    __vmx_vmwrite( VMCS_CTRL_MSR_BITMAP_ADDRESS, ProcessorState->BitmapMsrPhysical );

    //__vmx_vmwrite( VMCS_CTRL_IO_BITMAP_A_ADDRESS, 0 );
    //__vmx_vmwrite( VMCS_CTRL_IO_BITMAP_B_ADDRESS, 0 );

    //__vmx_vmwrite( VMCS_CTRL_EXCEPTION_BITMAP, 0 );

    __vmx_vmwrite( VMCS_CTRL_VMEXIT_CONTROLS, HvAdjustBitControl( VM_EXIT_IA32E_MODE, IA32_VMX_TRUE_EXIT_CTLS ) );
    __vmx_vmwrite( VMCS_CTRL_VMEXIT_MSR_STORE_COUNT, 0 );
    __vmx_vmwrite( VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT, 0 );

    __vmx_vmwrite( VMCS_CTRL_VMENTRY_CONTROLS, HvAdjustBitControl( VM_ENTRY_IA32E_MODE, IA32_VMX_TRUE_ENTRY_CTLS ) );
    __vmx_vmwrite( VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 0 );
    __vmx_vmwrite( VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, 0 );
    __vmx_vmwrite( VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, 0 );

    __vmx_vmwrite( VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, 1 );

    __vmx_vmwrite( VMCS_CTRL_EPT_POINTER, ProcessorState->EptPointer.Value );

    VmxInitializeProcessorHostControl( ProcessorState, &ProcessorDescriptor );
    VmxInitializeProcessorGuestControl( ProcessorState, &ProcessorDescriptor );

    return HVSTATUS_SUCCESS;
}
