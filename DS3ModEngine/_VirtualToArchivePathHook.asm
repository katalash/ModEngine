EXTERN ReplaceFileLoadPath:QWORD

.data

.code

    VirtualToArchivePathEpilogueHook proc
    
	push rbx
	push rbp
	push rsi
	push r12
	push r14
	push rcx
	push rcx
	push rcx
	push rcx
	push rcx
	push rcx
	push rcx
	sub rsp, 20h
	mov rcx, rax
	lea rax, ReplaceFileLoadPath
	call rax
	add rsp, 20h
	pop rcx
	pop rcx
	pop rcx
	pop rcx
	pop rcx
	pop rcx
	pop rcx
	pop r14
	pop r12
	pop rsi
	pop rbp
	pop rbx
	ret

    VirtualToArchivePathEpilogueHook endp

END

