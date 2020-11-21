


.DATA

ResumeFault db "vmresume fault", 10, 0
LaunchFault db "vmresume fault", 10, 0

.CODE

PUBLIC VmxHandleExitInternal
PUBLIC VmxLaunchAndStore
PUBLIC VmxLaunchAndRestore

PUBLIC @exit_resume
PUBLIC @exit_vmx

;
;   exitf.asm
;
 
EXTERNDEF ExfHandleCpuid:PROC
EXTERNDEF ExfHandleRdmsr:PROC
EXTERNDEF ExfHandleWrmsr:PROC
EXTERNDEF ExfHandleSetbv:PROC
EXTERNDEF ExfHandleInvd:PROC

EXTERNDEF DbgPrint:PROC
EXTERNDEF KeBugCheck:PROC
EXTERNDEF VmxHandleExit:PROC

HYPERVISOR_ERROR = 20001h

ALIGN 16

VmxHandleExitInternal PROC FRAME

    ;
    ;   vm passes control to this function on vmexit.
    ;
    
    sub     rsp, 28h + (16 * 8)
    .ALLOCSTACK  28h + (16 * 8)
    .ENDPROLOG

    ;
    ;   save rax and attempt to use an exitf
    ;   function instead, rax is needed for the vmread.
    ;

    mov     qword ptr [rsp + 28h + 50h], r10
    mov     qword ptr [rsp + 28h + 58h], r11

    mov     r10, 4402h                          ; exit reason
    vmread  r10, r10
    cmp     r10b, 0Ah
    jz      ExfHandleCpuid
    cmp     r10b, 0Dh
    jz      ExfHandleInvd
    cmp     r10b, 37h
    jz      ExfHandleSetbv
    cmp     r10b, 1Fh
    jz      ExfHandleRdmsr
    cmp     r10b, 20h
    jz      ExfHandleWrmsr

    mov     qword ptr [rsp + 28h + 00h], rax
    mov     qword ptr [rsp + 28h + 08h], rcx
    mov     qword ptr [rsp + 28h + 10h], rdx
    mov     qword ptr [rsp + 28h + 18h], rbx
    mov     qword ptr [rsp + 28h + 28h], rbp
    mov     qword ptr [rsp + 28h + 30h], rsi
    mov     qword ptr [rsp + 28h + 38h], rdi
    mov     qword ptr [rsp + 28h + 40h], r8
    mov     qword ptr [rsp + 28h + 48h], r9
    ; mov     qword ptr [rsp + 28h + 50h], r10
    ; mov     qword ptr [rsp + 28h + 58h], r11
    mov     qword ptr [rsp + 28h + 60h], r12
    mov     qword ptr [rsp + 28h + 68h], r13
    mov     qword ptr [rsp + 28h + 70h], r14
    mov     qword ptr [rsp + 28h + 78h], r15

    lea     rcx, [rsp + 28h]
    call    VmxHandleExit

    test    rax, rax
    jnz     @exit_vmx

    mov     rax, qword ptr [rsp + 28h + 00h]
    mov     rcx, qword ptr [rsp + 28h + 08h]
    mov     rdx, qword ptr [rsp + 28h + 10h]
    mov     rbx, qword ptr [rsp + 28h + 18h]
    mov     rbp, qword ptr [rsp + 28h + 28h]
    mov     rsi, qword ptr [rsp + 28h + 30h]
    mov     rdi, qword ptr [rsp + 28h + 38h]
    mov     r8,  qword ptr [rsp + 28h + 40h]
    mov     r9,  qword ptr [rsp + 28h + 48h]
    mov     r10, qword ptr [rsp + 28h + 50h]
    mov     r11, qword ptr [rsp + 28h + 58h]
    mov     r12, qword ptr [rsp + 28h + 60h]
    mov     r13, qword ptr [rsp + 28h + 68h]
    mov     r14, qword ptr [rsp + 28h + 70h]
    mov     r15, qword ptr [rsp + 28h + 78h]

@exit_resume LABEL FAR

    add     rsp, 28h + (16 * 8)
    vmresume
    sub     rsp, 28h + (16 * 8)

    ;
    ;   if code got here something died lol 
    ;   TODO:   change this to return to wherever 
    ;           we left off (which should be the 
    ;           vmlaunch instruction).
    ;

@fault:
    mov     rdx, 4400h
    vmread  qword ptr [rsp], rdx
    mov     rdx, qword ptr [rsp]
    vmxoff

    mov     rcx, offset ResumeFault
    call    DbgPrint

    mov     ecx, HYPERVISOR_ERROR
    call    KeBugCheck

@exit_vmx LABEL FAR

    ;
    ;   if this is being called, the exit reason is likely
    ;   a large fault, or a request to vmxoff. 
    ;   restore the guest state, and jump back to where ever
    ;   we left off.
    ;
    
    mov     rcx, 6802h                          ; guest cr3
    vmread  rdx, rcx
    mov     cr3, rdx

    mov     rdx, qword ptr [rsp + 28h + 20h]
    sub     rdx, 16
    mov     rcx, 681Eh                          ; guest ip
    vmread  qword ptr [rdx+8], rcx
    mov     rcx, 6820h                          ; guest eflags
    vmread  qword ptr [rdx], rcx
    mov     qword ptr [rsp + 28h + 20h], rdx

    mov     rax, qword ptr [rsp + 28h + 00h]
    mov     rcx, qword ptr [rsp + 28h + 08h]
    mov     rdx, qword ptr [rsp + 28h + 10h]
    mov     rbx, qword ptr [rsp + 28h + 18h]
    mov     rbp, qword ptr [rsp + 28h + 28h]
    mov     rsi, qword ptr [rsp + 28h + 30h]
    mov     rdi, qword ptr [rsp + 28h + 38h]
    mov     r8,  qword ptr [rsp + 28h + 40h]
    mov     r9,  qword ptr [rsp + 28h + 48h]
    mov     r10, qword ptr [rsp + 28h + 50h]
    mov     r11, qword ptr [rsp + 28h + 58h]
    mov     r12, qword ptr [rsp + 28h + 60h]
    mov     r13, qword ptr [rsp + 28h + 68h]
    mov     r14, qword ptr [rsp + 28h + 70h]
    mov     r15, qword ptr [rsp + 28h + 78h]
    mov     rsp, qword ptr [rsp + 28h + 20h]
    popfq

    vmxoff
    ret

VmxHandleExitInternal ENDP

ALIGN 16

VmxLaunchAndStore PROC FRAME

    ;
    ;   store the current stack in the control 
    ;   with the return address at the top
    ;   and then clear rax for HVSTATUS_SUCCESS
    ;

    mov     rcx, 681Ch
    vmwrite rcx, rsp

    xor     rax, rax
    vmlaunch

    ;
    ;   if code got here, something went wrong.
    ;   just abort, no need to bugcheck.
    ;

    sub     rsp, 28h
    .ALLOCSTACK  28h
    .ENDPROLOG
    
    mov     rdx, 4400h
    vmread  qword ptr [rsp], rdx
    mov     rdx, qword ptr [rsp]
    vmxoff

    mov     rcx, offset LaunchFault
    call    DbgPrint

    or      ax, 1
    ret

VmxLaunchAndStore ENDP

ALIGN 16

VmxLaunchAndRestore PROC FRAME
    .ENDPROLOG

    ;
    ;   this procedure is the guest's initial ip.
    ;   when vmlaunch is executed, successfully, it will
    ;   jump here, rax will be set appropriately, either
    ;   by the vmexit (for a triple fault, ept misconfig
    ;   or other hard fault) which should store unsucessful
    ;   in rax, or it will be set by VmxLaunchAndStore upon a
    ;   successful launch, the return address is on the top of
    ;   the stack.
    ;

    ret

VmxLaunchAndRestore ENDP

END
