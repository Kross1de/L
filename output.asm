section .text
global _start
extern pow

global subtractNumber
subtractNumber:
    push rbp
    mov rbp, rsp
    ; Parameter a
    sub rsp, 8
    mov [rbp - ]a], rdi
    ; Parameter b
    sub rsp, 8
    mov [rbp - ]b], rsi
    fld qword [rbp - a]
    fld qword [rbp - b]
    fsubp
    fstp dword [rsp - 8]
    movsd xmm0, [rsp - 8]
    add rsp, 8
    mov rsp, rbp
    pop rbp
    ret