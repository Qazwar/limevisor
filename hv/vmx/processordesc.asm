


.CODE

PUBLIC VmxGetProcessorDescriptor


ALIGN 16

VmxGetProcessorDescriptor PROC FRAME
    
    .ENDPROLOG

    mov     rax, cs
    mov     qword ptr [rcx+00h], rax
    mov     rax, ss
    mov     qword ptr [rcx+08h], rax
    mov     rax, ds
    mov     qword ptr [rcx+10h], rax
    mov     rax, es
    mov     qword ptr [rcx+18h], rax
    mov     rax, fs
    mov     qword ptr [rcx+20h], rax
    mov     rax, gs
    mov     qword ptr [rcx+28h], rax
    str     rax
    mov     qword ptr [rcx+30h], rax
    sldt    rax
    mov     qword ptr [rcx+38h], rax
    sidt    oword ptr [rcx+40h]
    sgdt    oword ptr [rcx+4Ah]

    ret

VmxGetProcessorDescriptor ENDP

END