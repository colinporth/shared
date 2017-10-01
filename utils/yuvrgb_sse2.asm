; yuvrgb_sse2.asm - se2 yuv to rgb converter - 8 pixels per loop

SECTION .data align=16
; r = y + 0 * u + 1.402 * v
; g = y + -0.344 * u + -0.714 * v
; b = y + 1.772 * u + 0 * v
Const16    dw    16,    16,    16,    16,    16,    16,    16,    16
Const128   dw   128,   128,   128,   128,   128,   128,   128,   128
RConst     dw     0,  5743,     0,  5743,     0,  5743,     0,  5743
GConst     dw -1409, -2925, -1409, -2925, -1409, -2925, -1409, -2925
BConst     dw  7258,     0,  7258,     0,  7258,     0,  7258,     0

%ifdef WIN64
;-------------------------------------------------------- X64----------------------------------------------------
  DEFAULT REL
  BITS 64

; void yuv420_rgba32_sse2 (void* yPtr, void* uPtr, void* vPtr, void* toPtr, int64 width)
SECTION .text align=16
        ALIGN 16
        global yuv420_rgba32_sse2
yuv420_rgba32_sse2:              ; rcx,rdx,r8,r9,rax,xmm0-6 volatile regs
;                                  rcx = yPtr
;                                  rdx = uPtr
;                                   r8 = vPtr
;                                   r9 = toPtr
        mov rax, [rsp + 40]      ; rax = width
;                                ; loop over width/8, YUV420 Planar input
        shr  rax,3
        test rax,rax
        jng  ENDLOOP

REPEATLOOP:
        movq xmm0,[rcx]          ; fetch 8 y values 8bit yyyyyyyy00000000
        movd xmm3,[rdx]          ; fetch 4 u values 8bit uuuu000000000000
        movd xmm1,[r8]           ; fetch 4 v values 8bit vvvv000000000000

; extract y
        pxor xmm2,xmm2           ; 00000000000000000000000000000000
        punpcklbw xmm0,xmm2      ; interleave xmm2 into xmm0 y0y0y0y0y0y0y0y0
                                 ; combine u and v

        punpcklbw xmm3,xmm1      ; uvuvuvuv00000000
        punpcklbw xmm3,xmm2      ; u0v0u0v0u0v0u0v0
        psubsw xmm0,[Const16]    ; y = y - 16
        psubsw xmm3,[Const128]   ; u = u - 128, v = v -128

        movdqa xmm4,xmm3         ; duplicate
        pshufd xmm5,xmm3,0xE4    ; duplicate
                                 ; r = y + 0 * u + 1.402 * v
                                 ; g = y + -0.344 * u + -0.714 * v
                                 ; b = y + 1.772 * u + 0 * v
        pmaddwd xmm3,[RConst]    ; multiply and add
        pmaddwd xmm4,[GConst]    ; to get RGB offsets to Y
        pmaddwd xmm5,[BConst]

        psrad xmm3,12            ; Scale back to original range
        psrad xmm4,12
        psrad xmm5,12

        pshuflw xmm3,xmm3,0xa0   ; duplicate results
        pshufhw xmm3,xmm3,0xa0
        pshuflw xmm4,xmm4,0xa0
        pshufhw xmm4,xmm4,0xa0
        pshuflw xmm5,xmm5,0xa0
        pshufhw xmm5,xmm5,0xa0

        paddsw xmm3,xmm0         ; add to y
        paddsw xmm4,xmm0
        paddsw xmm5,xmm0

; clamp values
        pcmpeqd  xmm2,xmm2       ; set all bits
        packuswb xmm3,xmm2       ; clamp to 0,255 pack R 8bit pixel
        packuswb xmm4,xmm2       ; clamp to 0,255 pack G 8bit pixel
        packuswb xmm5,xmm2       ; clamp to 0,255 pack B 8bit pixel

; convert to bgra32 packed
        punpcklbw xmm5,xmm4      ; bgbgbgbgbgbgbgbg
        movdqa xmm0, xmm5        ; save bg values
        punpcklbw xmm3,xmm2      ; r0r0r0r0r0r0r0r0
        punpcklwd xmm5,xmm3      ; lower half bgr0bgr0bgr0bgr0
        punpckhwd xmm0,xmm3      ; upper half bgr0bgr0bgr0bgr0

; write to output ptr
        movntdq [r9],xmm5        ; output 1st 4 pixels bypassing cache
        movntdq [r9+16],xmm0     ; output 2nd 4 pixels bypassing cache

; endloop
        add r9,32                ; toPtr inc
        add rcx,8                ; yPtr inc
        add rdx,4                ; uPtr inc
        add r8,4                 ; vPtr inc
        sub rax,1
        jnz REPEATLOOP
ENDLOOP:
        ret
%endif

%ifdef X86_32
;-----------------------------------------------x86-------------------------------------------------------
; void yuv420_rgba32_sse2 (void *fromYPtr, void *fromUPtr, void *fromVPtr, void *toPtr, int width)
%define fromYPtr ebp+8
%define fromUPtr ebp+12
%define fromVPtr ebp+16
%define toPtr    ebp+20
%define width    ebp+24

SECTION .text align=16
        ALIGN 16
        global _yuv420_rgba32_sse2
        %define yuv420_rgba32_sse2 _yuv420_rgba32_sse2

yuv420_rgba32_sse2:
; reserve variables
        push ebp
        mov ebp, esp
        push edi
        push esi
        push ecx
        push eax
        push ebx
; load params
        mov esi, [fromYPtr]
        mov eax, [fromUPtr]
        mov ebx, [fromVPtr]
        mov edi, [toPtr]
        mov ecx, [width]

; loop width/8 times
        shr ecx,3
        test ecx,ecx
        jng ENDLOOP

REPEATLOOP:                       ; loop over width / 8
; YUV420 Planar input
        movq xmm0, [esi]          ; fetch 8 y values (8 bit) yyyyyyyy00000000
        movd xmm3, [eax]          ; fetch 4 u values (8 bit) uuuu000000000000
        movd xmm1, [ebx]          ; fetch 4 v values (8 bit) vvvv000000000000

; extract y
        pxor xmm2, xmm2           ; 00000000000000000000000000000000
        punpcklbw xmm0, xmm2      ; interleave xmm2 into xmm0 y0y0y0y0y0y0y0y0

; combine u and v
        punpcklbw xmm3, xmm1      ; uvuvuvuv00000000
        punpcklbw xmm3, xmm2      ; u0v0u0v0u0v0u0v0

        psubsw xmm0, [Const16]     ; y = y - 16
        psubsw xmm3, [Const128]    ; u = u - 128, v = v -128

        movdqa xmm4, xmm3          ; duplicate
        pshufd xmm5, xmm3, 0xE4    ; duplicate

; r = y + 0 * u + 1.402 * v
; g = y + -0.344 * u + -0.714 * v
; b = y + 1.772 * u + 0 * v
        pmaddwd xmm3, [RConst]     ; multiply and add
        pmaddwd xmm4, [GConst]     ; to get RGB offsets to Y
        pmaddwd xmm5, [BConst]

        psrad xmm3, 12             ; Scale back to original range
        psrad xmm4, 12
        psrad xmm5, 12

        pshuflw xmm3, xmm3, 0xa0   ; duplicate results
        pshufhw xmm3, xmm3, 0xa0
        pshuflw xmm4, xmm4, 0xa0
        pshufhw xmm4, xmm4, 0xa0
        pshuflw xmm5, xmm5, 0xa0
        pshufhw xmm5, xmm5, 0xa0

        paddsw xmm3, xmm0          ; add to y
        paddsw xmm4, xmm0
        paddsw xmm5, xmm0

; clamp values
        pcmpeqd  xmm2,xmm2         ; set all bits
        packuswb xmm3,xmm2         ; clamp to 0,255 and pack R to 8 bit per pixel
        packuswb xmm4,xmm2         ; clamp to 0,255 and pack G to 8 bit per pixel
        packuswb xmm5,xmm2         ; clamp to 0,255 and pack B to 8 bit per pixel

; convert to bgra32 packed
        punpcklbw xmm5,xmm4        ; bgbgbgbgbgbgbgbg
        movdqa xmm0, xmm5          ; save bg values
        punpcklbw xmm3,xmm2        ; r0r0r0r0r0r0r0r0
        punpcklwd xmm5,xmm3        ; lower half bgr0bgr0bgr0bgr0
        punpckhwd xmm0,xmm3        ; upper half bgr0bgr0bgr0bgr0

; write to output ptr
        movntdq [edi], xmm5        ; output first 4 pixels bypassing cache
        movntdq [edi+16], xmm0     ; output second 4 pixels bypassing cache

; endloop
        add edi,32
        add esi,8
        add eax,4
        add ebx,4
        sub ecx, 1                 ; apparently sub is better than dec
        jnz REPEATLOOP
ENDLOOP:

; cleanup
        pop ebx
        pop eax
        pop ecx
        pop esi
        pop edi
        mov esp, ebp
        pop ebp
        ret
%endif

SECTION .note.GNU-stack noalloc noexec nowrite progbits
