


.CODE

;
;   a lot of these instructions dont need extra processing, 
;   so we can use this to optimize.
;

PUBLIC ExfHandleCpuid
PUBLIC ExfHandleRdmsr
PUBLIC ExfHandleWrmsr
PUBLIC ExfHandleSetbv
PUBLIC ExfHandleInvd

PUBLIC ExfHandleResume

EXTERNDEF @exit_resume:FAR
EXTERNDEF @exit_vmx:FAR

ALIGN 16
ExfHandleCpuid PROC FRAME
    .ENDPROLOG
    cmp     eax, 0D3ADB00Bh
    jz      ExfHandleExitVmx
    cmp     eax, 00000001h      ; version info
    jnz     @regular_cpuid
    cpuid
    btr     cx, 5               ; clear vmx bit
    jmp     ExfHandleResume
@regular_cpuid:
    cpuid
    jmp     ExfHandleResume
ExfHandleCpuid ENDP

ALIGN 16
ExfHandleRdmsr PROC FRAME
    .ENDPROLOG
    rdmsr 
    jmp     ExfHandleResume
ExfHandleRdmsr ENDP

ALIGN 16
ExfHandleWrmsr PROC FRAME
    .ENDPROLOG
    rdmsr 
    jmp     ExfHandleResume
ExfHandleWrmsr ENDP

ALIGN 16
ExfHandleSetbv PROC FRAME
    .ENDPROLOG
    xsetbv
    jmp     ExfHandleResume
ExfHandleSetbv ENDP

ALIGN 16
ExfHandleInvd PROC FRAME
    .ENDPROLOG
    wbinvd
    jmp     ExfHandleResume
ExfHandleInvd ENDP

ALIGN 16
ExfHandleResume PROC FRAME
    .ENDPROLOG
    mov     r10, 681Eh                          ; guest rip
    vmread  r10, r10
    mov     r11, 440Ch                          ; instruction length
    vmread  r11, r11
    add     r10, r11
    mov     r11, 681Eh                          ; guest rip
    vmwrite r10, r11
    mov     r10, qword ptr [rsp + 28h + 50h]
    mov     r11, qword ptr [rsp + 28h + 58h]
    jmp     @exit_resume
ExfHandleResume ENDP

ALIGN 16
ExfHandleExitVmx PROC FRAME
    .ENDPROLOG
    mov     r10, 681Ch
    vmread  r10, r10
    mov     qword ptr [rsp + 28h + 20h], r10
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
    jmp     @exit_vmx
ExfHandleExitVmx ENDP

END