; ============================================================================
;  stack_switch.asm —— 把 RSP 切到自备大栈上再调用目标函数
;
;  为什么需要：Calcium 的脚本执行器（_ckapi_scriptload / _runcode_api /
;  _script<>）是深递归 + 大栈帧，固件给应用分配的栈只有几百 KB，实测脚本
;  嵌套 5 层就栈溢出（表现为三重故障直接复位，连异常处理都进不去）。
;
;  extern "C" unsigned long long
;  efi_switch_stack(void *new_top, int (*fn)(int, char**), int argc, char **argv);
;
;  RCX = new_top   RDX = fn   R8D = argc   R9 = argv
; ============================================================================

.code

efi_switch_stack PROC FRAME
    push    rbp
    .pushreg rbp
    push    rbx
    .pushreg rbx
    push    rdi
    .pushreg rdi
    push    rsi
    .pushreg rsi
    push    r12
    .pushreg r12
    push    r13
    .pushreg r13
    push    r14
    .pushreg r14
    push    r15
    .pushreg r15
    .endprolog

    mov     rbx, rsp            ; 记住原栈指针
    mov     r12, rdx            ; fn
    mov     r13d, r8d           ; argc
    mov     r14, r9             ; argv

    and     rcx, -16            ; 新栈顶按 16 字节对齐
    mov     rsp, rcx
    sub     rsp, 32             ; 32 字节 shadow space

    mov     ecx, r13d
    mov     rdx, r14
    call    r12

    mov     rsp, rbx            ; 回到原栈
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rsi
    pop     rdi
    pop     rbx
    pop     rbp
    ret

efi_switch_stack ENDP

END
