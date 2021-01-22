


#pragma once


typedef struct _VMX_EXIT_TRAP_FRAME {
    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;

} VMX_EXIT_TRAP_FRAME, *PVMX_EXIT_TRAP_FRAME;


typedef struct _VMX_SEGMENT_DESCRIPTOR {
    ULONG64     SegmentBase;
    ULONG64     SegmentLimit;

    union {
        struct {

            ULONG32 Type : 4;
            ULONG32 DescriptorType : 1;
            ULONG32 DescriptorPrivilegeLevel : 2;
            ULONG32 Present : 1;
            ULONG32 Reserved1 : 4;
            ULONG32 Available : 1;
            ULONG32 LongMode : 1;
            ULONG32 DefaultBig : 1;
            ULONG32 Granularity : 1;
            ULONG32 Unusable : 1;
            ULONG32 Reserved2 : 15;
        };

        ULONG32 Value;
    } AccessRights;

} VMX_SEGMENT_DESCRIPTOR, *PVMX_SEGMENT_DESCRIPTOR;

C_ASSERT( sizeof( VMX_SEGMENT_DESCRIPTOR ) == 20 );


typedef struct _PHYSICAL_REGION_DESCRIPTOR {
    ULONG64 RegionBase;
    ULONG64 RegionLength;
    ULONG64 RegionType;

} PHYSICAL_REGION_DESCRIPTOR, *PPHYSICAL_REGION_DESCRIPTOR;


typedef struct _VMX_PAGE_TABLE_BASE {

    //
    //  credit goes to gbhv for this idea, it's a structure 
    //  dedicated to identity mapping 512 GB
    //
    //  NOTE: you could change these PML3's for 1GB PML3's
    //

    EPT_PML Level4[ 512 ];
    EPT_PML Level3[ 512 ];
    EPT_PML Level2[ 512 ][ 512 ];

} VMX_PAGE_TABLE_BASE, *PVMX_PAGE_TABLE_BASE;


typedef struct _VMX_PAGE_TABLE {
    EPT_PML     PageEntry[ 512 ];
    PEPT_PML    PageEntryParent;
    ULONG64     PhysicalBaseAddress;
    LIST_ENTRY  TableLinks;

} VMX_PAGE_TABLE, *PVMX_PAGE_TABLE;


typedef struct _VMX_PCB {
    ULONG64              OnRegion;
    ULONG64              OnRegionPhysical;

    ULONG64              ControlRegion;
    ULONG64              ControlRegionPhysical;

    ULONG64              HostStack;
    ULONG64              HostStackSize;

    ULONG64              MsrMap;
    ULONG64              MsrMapPhysical;

    EPT_POINTER          EptPointer;
    PVMX_PAGE_TABLE_BASE PageTable;

    PLIST_ENTRY          HookHead;
    PLIST_ENTRY          TableHead;

} VMX_PCB, *PVMX_PCB;

#pragma pack(push, 8)
typedef struct _VMX_EXIT_STATE {
    HVSTATUS    FinalStatus;
    ULONG64     ExitReason;
    ULONG64     ExitQualification;
    ULONG64     CurrentIp;
    BOOLEAN     IncrementIp;

} VMX_EXIT_STATE, *PVMX_EXIT_STATE;
#pragma pack(pop)

typedef struct _SEGMENT_DESCRIPTOR_REGISTER {
    USHORT      Limit;
    ULONG64     BaseAddress;

} SEGMENT_DESCRIPTOR_REGISTER, *PSEGMENT_DESCRIPTOR_REGISTER;

typedef struct _VMX_PROCESSOR_DESCRIPTOR {
    ULONG64 SegCS;
    ULONG64 SegSS;
    ULONG64 SegDS;
    ULONG64 SegES;
    ULONG64 SegFS;
    ULONG64 SegGS;
    ULONG64 TaskRegister;
    ULONG64 LdtRegister;

    SEGMENT_DESCRIPTOR_REGISTER IdtRegister;
    SEGMENT_DESCRIPTOR_REGISTER GdtRegister;

} VMX_PROCESSOR_DESCRIPTOR, *PVMX_PROCESSOR_DESCRIPTOR;

typedef struct _SEGMENT_DESCRIPTOR {
    USHORT SegmentLimitLow;
    USHORT BaseAddressLow;

    union {
        struct {
            ULONG32 BaseAddressMiddle : 8;
            ULONG32 Type : 4;
            ULONG32 DescriptorType : 1;
            ULONG32 DescriptorPrivilegeLevel : 2;
            ULONG32 Present : 1;
            ULONG32 SegmentLimitHigh : 4;
            ULONG32 System : 1;
            ULONG32 LongMode : 1;
            ULONG32 DefaultBig : 1;
            ULONG32 Granularity : 1;
            ULONG32 BaseAddressHigh : 8;
        };

        ULONG32     Long;
    };

    ULONG32 BaseAddressUpper;
    ULONG32 Zero;
} SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;

#define VMX_EXIT_QUALIFICATION_ACCESS_MOV_TO_CR                      0x00000000
#define VMX_EXIT_QUALIFICATION_ACCESS_MOV_FROM_CR                    0x00000001
#define VMX_EXIT_QUALIFICATION_ACCESS_CLTS                           0x00000002
#define VMX_EXIT_QUALIFICATION_ACCESS_LMSW                           0x00000003

typedef union _VMX_EQ_ACCESS_CONTROL {
    struct {
        ULONG64 ControlRegister : 4;
        ULONG64 AccessType : 2;
        ULONG64 LmswMemoryOperand : 1; // 1 = memory operand, 0 = register
        ULONG64 Reserved1 : 1;
        ULONG64 Register : 4;
        ULONG64 Reserved2 : 4;
        ULONG64 LmswSourceData : 16;
        ULONG64 Reserved3 : 32;
    };

    ULONG64     Long;
} VMX_EQ_ACCESS_CONTROL, *PVMX_EQ_ACCESS_CONTROL;

typedef enum _VMX_INTERRUPT_TYPE {
    VmxInterruptExternal = 0,
    VmxInterruptNonMaskable = 2,
    VmxInterruptHardwareException = 3,
    VmxInterruptSoftware = 4,
    VmxInterruptPrivilegedSoftware = 5,
    VmxInterruptSoftwareException = 6,
    VmxInterruptOtherEvent = 7,
    VmxInterruptMaximum = 7
} VMX_INTERRUPT_TYPE;

typedef union _VMX_INTERRUPT_INFORMATION {
    struct {
        ULONG32 Vector : 8;
        ULONG32 Type : 3;
        ULONG32 ErrorCode : 1;
        ULONG32 Reserved1 : 19;
        ULONG32 Valid : 1;
    };

    ULONG32     Long;
} VMX_INTERRUPT_INFORMATION, *PVMX_INTERRUPT_INFORMATION;

#define IA32_FEATURE_CONTROL                                         0x0000003A

#define IA32_DEBUGCTL                                                0x000001D9

#define IA32_MTRR_CAPABILITIES                                       0x000000FE
#define IA32_MTRR_PHYSBASE0                                          0x00000200
#define IA32_MTRR_PHYSMASK0                                          0x00000201
#define IA32_MTRR_DEF_TYPE                                           0x000002FF


#define IA32_VMX_BASIC                                               0x00000480
#define IA32_VMX_PINBASED_CTLS                                       0x00000481
#define IA32_VMX_PROCBASED_CTLS                                      0x00000482
#define IA32_VMX_PROCBASED_CTLS2                                     0x0000048B
#define IA32_VMX_EPT_VPID_CAP                                        0x0000048C
#define IA32_VMX_TRUE_PINBASED_CTLS                                  0x0000048D
#define IA32_VMX_TRUE_PROCBASED_CTLS                                 0x0000048E
#define IA32_VMX_TRUE_EXIT_CTLS                                      0x0000048F
#define IA32_VMX_TRUE_ENTRY_CTLS                                     0x00000490

#define IA32_VMX_CR0_FIXED0                                          0x00000486
#define IA32_VMX_CR0_FIXED1                                          0x00000487
#define IA32_VMX_CR4_FIXED0                                          0x00000488
#define IA32_VMX_CR4_FIXED1                                          0x00000489

#define IA32_SYSENTER_CS                                             0x00000174
#define IA32_SYSENTER_ESP                                            0x00000175
#define IA32_SYSENTER_EIP                                            0x00000176

#define IA32_FS_BASE                                                 0xC0000100
#define IA32_GS_BASE                                                 0xC0000101
#define IA32_KERNEL_GS_BASE                                          0xC0000102

#define VMX_EXIT_REASON_EXCEPTION_OR_NMI                             0x00000000
#define VMX_EXIT_REASON_EXTERNAL_INTERRUPT                           0x00000001
#define VMX_EXIT_REASON_TRIPLE_FAULT                                 0x00000002
#define VMX_EXIT_REASON_INIT_SIGNAL                                  0x00000003
#define VMX_EXIT_REASON_STARTUP_IPI                                  0x00000004
#define VMX_EXIT_REASON_IO_SMI                                       0x00000005
#define VMX_EXIT_REASON_SMI                                          0x00000006
#define VMX_EXIT_REASON_INTERRUPT_WINDOW                             0x00000007
#define VMX_EXIT_REASON_NMI_WINDOW                                   0x00000008
#define VMX_EXIT_REASON_TASK_SWITCH                                  0x00000009
#define VMX_EXIT_REASON_EXECUTE_CPUID                                0x0000000A
#define VMX_EXIT_REASON_EXECUTE_GETSEC                               0x0000000B
#define VMX_EXIT_REASON_EXECUTE_HLT                                  0x0000000C
#define VMX_EXIT_REASON_EXECUTE_INVD                                 0x0000000D
#define VMX_EXIT_REASON_EXECUTE_INVLPG                               0x0000000E
#define VMX_EXIT_REASON_EXECUTE_RDPMC                                0x0000000F
#define VMX_EXIT_REASON_EXECUTE_RDTSC                                0x00000010
#define VMX_EXIT_REASON_EXECUTE_RSM_IN_SMM                           0x00000011
#define VMX_EXIT_REASON_EXECUTE_VMCALL                               0x00000012
#define VMX_EXIT_REASON_EXECUTE_VMCLEAR                              0x00000013
#define VMX_EXIT_REASON_EXECUTE_VMLAUNCH                             0x00000014
#define VMX_EXIT_REASON_EXECUTE_VMPTRLD                              0x00000015
#define VMX_EXIT_REASON_EXECUTE_VMPTRST                              0x00000016
#define VMX_EXIT_REASON_EXECUTE_VMREAD                               0x00000017
#define VMX_EXIT_REASON_EXECUTE_VMRESUME                             0x00000018
#define VMX_EXIT_REASON_EXECUTE_VMWRITE                              0x00000019
#define VMX_EXIT_REASON_EXECUTE_VMXOFF                               0x0000001A
#define VMX_EXIT_REASON_EXECUTE_VMXON                                0x0000001B
#define VMX_EXIT_REASON_MOV_CR                                       0x0000001C
#define VMX_EXIT_REASON_MOV_DR                                       0x0000001D
#define VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION                       0x0000001E
#define VMX_EXIT_REASON_EXECUTE_RDMSR                                0x0000001F
#define VMX_EXIT_REASON_EXECUTE_WRMSR                                0x00000020
#define VMX_EXIT_REASON_ERROR_INVALID_GUEST_STATE                    0x00000021
#define VMX_EXIT_REASON_ERROR_MSR_LOAD                               0x00000022
#define VMX_EXIT_REASON_EXECUTE_MWAIT                                0x00000024
#define VMX_EXIT_REASON_MONITOR_TRAP_FLAG                            0x00000025
#define VMX_EXIT_REASON_EXECUTE_MONITOR                              0x00000027
#define VMX_EXIT_REASON_EXECUTE_PAUSE                                0x00000028
#define VMX_EXIT_REASON_ERROR_MACHINE_CHECK                          0x00000029
#define VMX_EXIT_REASON_TPR_BELOW_THRESHOLD                          0x0000002B
#define VMX_EXIT_REASON_APIC_ACCESS                                  0x0000002C
#define VMX_EXIT_REASON_VIRTUALIZED_EOI                              0x0000002D
#define VMX_EXIT_REASON_GDTR_IDTR_ACCESS                             0x0000002E
#define VMX_EXIT_REASON_LDTR_TR_ACCESS                               0x0000002F
#define VMX_EXIT_REASON_EPT_VIOLATION                                0x00000030
#define VMX_EXIT_REASON_EPT_MISCONFIGURATION                         0x00000031
#define VMX_EXIT_REASON_EXECUTE_INVEPT                               0x00000032
#define VMX_EXIT_REASON_EXECUTE_RDTSCP                               0x00000033
#define VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED                 0x00000034
#define VMX_EXIT_REASON_EXECUTE_INVVPID                              0x00000035
#define VMX_EXIT_REASON_EXECUTE_WBINVD                               0x00000036
#define VMX_EXIT_REASON_EXECUTE_XSETBV                               0x00000037
#define VMX_EXIT_REASON_APIC_WRITE                                   0x00000038
#define VMX_EXIT_REASON_EXECUTE_RDRAND                               0x00000039
#define VMX_EXIT_REASON_EXECUTE_INVPCID                              0x0000003A
#define VMX_EXIT_REASON_EXECUTE_VMFUNC                               0x0000003B
#define VMX_EXIT_REASON_EXECUTE_ENCLS                                0x0000003C
#define VMX_EXIT_REASON_EXECUTE_RDSEED                               0x0000003D
#define VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL                   0x0000003E
#define VMX_EXIT_REASON_EXECUTE_XSAVES                               0x0000003F
#define VMX_EXIT_REASON_EXECUTE_XRSTORS                              0x00000040
#define VMX_EXIT_REASON_MAXIMUM                                      0x00000041

#define VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER                       0x00000000

#define VMCS_CTRL_POSTED_INTERRUPT_NOTIFICATION_VECTOR               0x00000002

#define VMCS_CTRL_EPTP_INDEX                                         0x00000004

#define VMCS_GUEST_ES_SELECTOR                                       0x00000800
#define VMCS_GUEST_CS_SELECTOR                                       0x00000802
#define VMCS_GUEST_SS_SELECTOR                                       0x00000804
#define VMCS_GUEST_DS_SELECTOR                                       0x00000806
#define VMCS_GUEST_FS_SELECTOR                                       0x00000808
#define VMCS_GUEST_GS_SELECTOR                                       0x0000080A
#define VMCS_GUEST_LDTR_SELECTOR                                     0x0000080C
#define VMCS_GUEST_TR_SELECTOR                                       0x0000080E
#define VMCS_GUEST_INTERRUPT_STATUS                                  0x00000810
#define VMCS_GUEST_PML_INDEX                                         0x00000812

#define VMCS_HOST_ES_SELECTOR                                        0x00000C00
#define VMCS_HOST_CS_SELECTOR                                        0x00000C02
#define VMCS_HOST_SS_SELECTOR                                        0x00000C04
#define VMCS_HOST_DS_SELECTOR                                        0x00000C06
#define VMCS_HOST_FS_SELECTOR                                        0x00000C08
#define VMCS_HOST_GS_SELECTOR                                        0x00000C0A
#define VMCS_HOST_TR_SELECTOR                                        0x00000C0C

#define VMCS_CTRL_IO_BITMAP_A_ADDRESS                                0x00002000
#define VMCS_CTRL_IO_BITMAP_B_ADDRESS                                0x00002002
#define VMCS_CTRL_MSR_BITMAP_ADDRESS                                 0x00002004
#define VMCS_CTRL_VMEXIT_MSR_STORE_ADDRESS                           0x00002006
#define VMCS_CTRL_VMEXIT_MSR_LOAD_ADDRESS                            0x00002008
#define VMCS_CTRL_VMENTRY_MSR_LOAD_ADDRESS                           0x0000200A
#define VMCS_CTRL_EXECUTIVE_VMCS_POINTER                             0x0000200C
#define VMCS_CTRL_PML_ADDRESS                                        0x0000200E
#define VMCS_CTRL_TSC_OFFSET                                         0x00002010
#define VMCS_CTRL_VIRTUAL_APIC_ADDRESS                               0x00002012
#define VMCS_CTRL_APIC_ACCESS_ADDRESS                                0x00002014
#define VMCS_CTRL_POSTED_INTERRUPT_DESCRIPTOR_ADDRESS                0x00002016
#define VMCS_CTRL_VMFUNC_CONTROLS                                    0x00002018
#define VMCS_CTRL_EPT_POINTER                                        0x0000201A
#define VMCS_CTRL_EOI_EXIT_BITMAP_0                                  0x0000201C
#define VMCS_CTRL_EOI_EXIT_BITMAP_1                                  0x0000201E
#define VMCS_CTRL_EOI_EXIT_BITMAP_2                                  0x00002020
#define VMCS_CTRL_EOI_EXIT_BITMAP_3                                  0x00002022
#define VMCS_CTRL_EPT_POINTER_LIST_ADDRESS                           0x00002024
#define VMCS_CTRL_VMREAD_BITMAP_ADDRESS                              0x00002026
#define VMCS_CTRL_VMWRITE_BITMAP_ADDRESS                             0x00002028
#define VMCS_CTRL_VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS       0x0000202A
#define VMCS_CTRL_XSS_EXITING_BITMAP                                 0x0000202C
#define VMCS_CTRL_ENCLS_EXITING_BITMAP                               0x0000202E
#define VMCS_CTRL_TSC_MULTIPLIER                                     0x00002032

#define VMCS_GUEST_PHYSICAL_ADDRESS                                  0x00002400

#define VMCS_GUEST_VMCS_LINK_POINTER                                 0x00002800
#define VMCS_GUEST_DEBUGCTL                                          0x00002802
#define VMCS_GUEST_PAT                                               0x00002804
#define VMCS_GUEST_EFER                                              0x00002806
#define VMCS_GUEST_PERF_GLOBAL_CTRL                                  0x00002808
#define VMCS_GUEST_PDPTE0                                            0x0000280A
#define VMCS_GUEST_PDPTE1                                            0x0000280C
#define VMCS_GUEST_PDPTE2                                            0x0000280E
#define VMCS_GUEST_PDPTE3                                            0x00002810
#define VMCS_GUEST_BNDCFGS                                           0x00002812
#define VMCS_GUEST_RTIT_CTL                                          0x00002814

#define VMCS_HOST_PAT                                                0x00002C00
#define VMCS_HOST_EFER                                               0x00002C02
#define VMCS_HOST_PERF_GLOBAL_CTRL                                   0x00002C04

#define VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS                    0x00004000
#define VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS              0x00004002
#define VMCS_CTRL_EXCEPTION_BITMAP                                   0x00004004
#define VMCS_CTRL_PAGEFAULT_ERROR_CODE_MASK                          0x00004006
#define VMCS_CTRL_PAGEFAULT_ERROR_CODE_MATCH                         0x00004008
#define VMCS_CTRL_CR3_TARGET_COUNT                                   0x0000400A
#define VMCS_CTRL_VMEXIT_CONTROLS                                    0x0000400C
#define VMCS_CTRL_VMEXIT_MSR_STORE_COUNT                             0x0000400E
#define VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT                              0x00004010
#define VMCS_CTRL_VMENTRY_CONTROLS                                   0x00004012
#define VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT                             0x00004014
#define VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD             0x00004016
#define VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE                       0x00004018
#define VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH                         0x0000401A
#define VMCS_CTRL_TPR_THRESHOLD                                      0x0000401C

#define VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS    0x0000401E

#define VMCS_CTRL_PLE_GAP                                            0x00004020
#define VMCS_CTRL_PLE_WINDOW                                         0x00004022

#define VMCS_VM_INSTRUCTION_ERROR                                    0x00004400

#define VMCS_EXIT_REASON                                             0x00004402

#define VMCS_VMEXIT_INTERRUPTION_INFORMATION                         0x00004404
#define VMCS_VMEXIT_INTERRUPTION_ERROR_CODE                          0x00004406

#define VMCS_IDT_VECTORING_INFORMATION                               0x00004408
#define VMCS_IDT_VECTORING_ERROR_CODE                                0x0000440A

#define VMCS_VMEXIT_INSTRUCTION_LENGTH                               0x0000440C
#define VMCS_VMEXIT_INSTRUCTION_INFO                                 0x0000440E

#define VMCS_GUEST_ES_LIMIT                                          0x00004800
#define VMCS_GUEST_CS_LIMIT                                          0x00004802
#define VMCS_GUEST_SS_LIMIT                                          0x00004804
#define VMCS_GUEST_DS_LIMIT                                          0x00004806
#define VMCS_GUEST_FS_LIMIT                                          0x00004808
#define VMCS_GUEST_GS_LIMIT                                          0x0000480A
#define VMCS_GUEST_LDTR_LIMIT                                        0x0000480C
#define VMCS_GUEST_TR_LIMIT                                          0x0000480E
#define VMCS_GUEST_GDTR_LIMIT                                        0x00004810
#define VMCS_GUEST_IDTR_LIMIT                                        0x00004812
#define VMCS_GUEST_ES_ACCESS_RIGHTS                                  0x00004814
#define VMCS_GUEST_CS_ACCESS_RIGHTS                                  0x00004816
#define VMCS_GUEST_SS_ACCESS_RIGHTS                                  0x00004818
#define VMCS_GUEST_DS_ACCESS_RIGHTS                                  0x0000481A
#define VMCS_GUEST_FS_ACCESS_RIGHTS                                  0x0000481C
#define VMCS_GUEST_GS_ACCESS_RIGHTS                                  0x0000481E
#define VMCS_GUEST_LDTR_ACCESS_RIGHTS                                0x00004820
#define VMCS_GUEST_TR_ACCESS_RIGHTS                                  0x00004822
#define VMCS_GUEST_INTERRUPTIBILITY_STATE                            0x00004824
#define VMCS_GUEST_ACTIVITY_STATE                                    0x00004826
#define VMCS_GUEST_SMBASE                                            0x00004828
#define VMCS_GUEST_SYSENTER_CS                                       0x0000482A

#define VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE                        0x0000482E

#define VMCS_HOST_SYSENTER_CS                                        0x00004C00

#define VMCS_CTRL_CR0_GUEST_HOST_MASK                                0x00006000
#define VMCS_CTRL_CR4_GUEST_HOST_MASK                                0x00006002
#define VMCS_CTRL_CR0_READ_SHADOW                                    0x00006004
#define VMCS_CTRL_CR4_READ_SHADOW                                    0x00006006
#define VMCS_CTRL_CR3_TARGET_VALUE_0                                 0x00006008
#define VMCS_CTRL_CR3_TARGET_VALUE_1                                 0x0000600A
#define VMCS_CTRL_CR3_TARGET_VALUE_2                                 0x0000600C
#define VMCS_CTRL_CR3_TARGET_VALUE_3                                 0x0000600E

#define VMCS_EXIT_QUALIFICATION                                      0x00006400

#define VMCS_IO_RCX                                                  0x00006402
#define VMCS_IO_RSX                                                  0x00006404
#define VMCS_IO_RDI                                                  0x00006406
#define VMCS_IO_RIP                                                  0x00006408

#define VMCS_EXIT_GUEST_LINEAR_ADDRESS                               0x0000640A

#define VMCS_GUEST_CR0                                               0x00006800
#define VMCS_GUEST_CR3                                               0x00006802
#define VMCS_GUEST_CR4                                               0x00006804
#define VMCS_GUEST_ES_BASE                                           0x00006806
#define VMCS_GUEST_CS_BASE                                           0x00006808
#define VMCS_GUEST_SS_BASE                                           0x0000680A
#define VMCS_GUEST_DS_BASE                                           0x0000680C
#define VMCS_GUEST_FS_BASE                                           0x0000680E
#define VMCS_GUEST_GS_BASE                                           0x00006810
#define VMCS_GUEST_LDTR_BASE                                         0x00006812
#define VMCS_GUEST_TR_BASE                                           0x00006814
#define VMCS_GUEST_GDTR_BASE                                         0x00006816
#define VMCS_GUEST_IDTR_BASE                                         0x00006818
#define VMCS_GUEST_DR7                                               0x0000681A
#define VMCS_GUEST_RSP                                               0x0000681C
#define VMCS_GUEST_RIP                                               0x0000681E
#define VMCS_GUEST_RFLAGS                                            0x00006820
#define VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS                          0x00006822
#define VMCS_GUEST_SYSENTER_ESP                                      0x00006824
#define VMCS_GUEST_SYSENTER_EIP                                      0x00006826

#define VMCS_HOST_CR0                                                0x00006C00
#define VMCS_HOST_CR3                                                0x00006C02
#define VMCS_HOST_CR4                                                0x00006C04
#define VMCS_HOST_FS_BASE                                            0x00006C06
#define VMCS_HOST_GS_BASE                                            0x00006C08
#define VMCS_HOST_TR_BASE                                            0x00006C0A
#define VMCS_HOST_GDTR_BASE                                          0x00006C0C
#define VMCS_HOST_IDTR_BASE                                          0x00006C0E
#define VMCS_HOST_SYSENTER_ESP                                       0x00006C10
#define VMCS_HOST_SYSENTER_EIP                                       0x00006C12
#define VMCS_HOST_RSP                                                0x00006C14
#define VMCS_HOST_RIP                                                0x00006C16

#define CPU_BASED_VIRTUAL_INTR_PENDING          0x00000004
#define CPU_BASED_USE_TSC_OFFSETING             0x00000008
#define CPU_BASED_HLT_EXITING                   0x00000080
#define CPU_BASED_INVLPG_EXITING                0x00000200
#define CPU_BASED_MWAIT_EXITING                 0x00000400
#define CPU_BASED_RDPMC_EXITING                 0x00000800
#define CPU_BASED_RDTSC_EXITING                 0x00001000
#define CPU_BASED_CR3_LOAD_EXITING              0x00008000
#define CPU_BASED_CR3_STORE_EXITING             0x00010000
#define CPU_BASED_CR8_LOAD_EXITING              0x00080000
#define CPU_BASED_CR8_STORE_EXITING             0x00100000
#define CPU_BASED_TPR_SHADOW                    0x00200000
#define CPU_BASED_VIRTUAL_NMI_PENDING           0x00400000
#define CPU_BASED_MOV_DR_EXITING                0x00800000
#define CPU_BASED_UNCOND_IO_EXITING             0x01000000
#define CPU_BASED_ACTIVATE_IO_BITMAP            0x02000000
#define CPU_BASED_MONITOR_TRAP_FLAG             0x08000000
#define CPU_BASED_ACTIVATE_MSR_BITMAP           0x10000000
#define CPU_BASED_MONITOR_EXITING               0x20000000
#define CPU_BASED_PAUSE_EXITING                 0x40000000
#define CPU_BASED_ACTIVATE_SECONDARY_CONTROLS   0x80000000

#define PIN_BASED_EXT_INTR_MASK                 0x00000001
#define PIN_BASED_NMI_EXITING                   0x00000008
#define PIN_BASED_VIRTUAL_NMIS                  0x00000020
#define PIN_BASED_PREEMPT_TIMER                 0x00000040
#define PIN_BASED_POSTED_INTERRUPT              0x00000080

#define VM_EXIT_SAVE_DEBUG_CNTRLS               0x00000004
#define VM_EXIT_IA32E_MODE                      0x00000200
#define VM_EXIT_LOAD_PERF_GLOBAL_CTRL           0x00001000
#define VM_EXIT_ACK_INTR_ON_EXIT                0x00008000
#define VM_EXIT_SAVE_GUEST_PAT                  0x00040000
#define VM_EXIT_LOAD_HOST_PAT                   0x00080000
#define VM_EXIT_SAVE_GUEST_EFER                 0x00100000
#define VM_EXIT_LOAD_HOST_EFER                  0x00200000
#define VM_EXIT_SAVE_PREEMPT_TIMER              0x00400000
#define VM_EXIT_CLEAR_BNDCFGS                   0x00800000

#define VM_ENTRY_IA32E_MODE                     0x00000200
#define VM_ENTRY_SMM                            0x00000400
#define VM_ENTRY_DEACT_DUAL_MONITOR             0x00000800
#define VM_ENTRY_LOAD_PERF_GLOBAL_CTRL          0x00002000
#define VM_ENTRY_LOAD_GUEST_PAT                 0x00004000
#define VM_ENTRY_LOAD_GUEST_EFER                0x00008000
#define VM_ENTRY_LOAD_BNDCFGS                   0x00010000
#define VM_ENTRY_CONCEAL_PIP                    0x00020000

#define SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES 0x00000001
#define SECONDARY_EXEC_ENABLE_EPT               0x00000002
#define SECONDARY_EXEC_DESCRIPTOR_TABLE_EXITING 0x00000004
#define SECONDARY_EXEC_ENABLE_RDTSCP            0x00000008
#define SECONDARY_EXEC_VIRTUALIZE_X2APIC_MODE   0x00000010
#define SECONDARY_EXEC_ENABLE_VPID              0x00000020
#define SECONDARY_EXEC_WBINVD_EXITING           0x00000040
#define SECONDARY_EXEC_UNRESTRICTED_GUEST       0x00000080
#define SECONDARY_EXEC_APIC_REGISTER_VIRT       0x00000100
#define SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY    0x00000200
#define SECONDARY_EXEC_PAUSE_LOOP_EXITING       0x00000400
#define SECONDARY_EXEC_ENABLE_INVPCID           0x00001000
#define SECONDARY_EXEC_ENABLE_VM_FUNCTIONS      0x00002000
#define SECONDARY_EXEC_ENABLE_VMCS_SHADOWING    0x00004000
#define SECONDARY_EXEC_ENABLE_PML               0x00020000
#define SECONDARY_EXEC_ENABLE_VIRT_EXCEPTIONS   0x00040000
#define SECONDARY_EXEC_XSAVES                   0x00100000
#define SECONDARY_EXEC_PCOMMIT                  0x00200000
#define SECONDARY_EXEC_TSC_SCALING              0x02000000

HVSTATUS
VmxInitializeProcessor(
    __in PVMX_PCB ProcessorState
);

HVSTATUS
VmxTerminateProcessor(
    __in PVMX_PCB ProcessorState
);

VOID
VmxGetProcessorDescriptor(
    __in PVMX_PROCESSOR_DESCRIPTOR ProcessorDescriptor
);

HVSTATUS
VmxInitializeProcessorGuestControl(
    __in PVMX_PCB      ProcessorState,
    __in PVMX_PROCESSOR_DESCRIPTOR ProcessorDescriptor
);

HVSTATUS
VmxInitializeProcessorHostControl(
    __in PVMX_PCB      ProcessorState,
    __in PVMX_PROCESSOR_DESCRIPTOR ProcessorDescriptor
);

HVSTATUS
VmxInitializeProcessorControl(
    __in PVMX_PCB ProcessorState
);

HVSTATUS
VmxLaunchAndStore(

);

HVSTATUS
VmxLaunchAndRestore(

);

HVSTATUS
VmxHandleExit(
    __in PVMX_EXIT_TRAP_FRAME TrapFrame
);

VOID
VmxHandleExitInternal(

);

