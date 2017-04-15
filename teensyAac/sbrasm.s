  .syntax unified

//{{{  void CVKernel1 (int* XBuf, int* accBuf)
  .section  .text.CVKernel1
  .type  CVKernel1, %function
  .global  CVKernel1
CVKernel1:
        stmfd   sp!, {r4-r11, r14}

        ldr     r3, [r0], #4*(1)
        ldr.w   r4, [r0, #4*(2*64-1)]
        ldr     r5, [r0], #4*(1)
        ldr.w   r6, [r0, #4*(2*64-1)]
        rsb     r14, r4, #0

        smull   r7, r8, r5, r3
        smlal   r7, r8, r6, r4
        smull   r9, r10, r3, r6
        smlal   r9, r10, r14, r5
        smull   r11, r12, r3, r3
        smlal   r11, r12, r4, r4

        add     r2, r1, #(4*6)
        stmia   r2, {r7-r12}

        mov     r7, #0
        mov     r8, #0
        mov     r9, #0
        mov     r10, #0
        mov     r11, #0
        mov     r12, #0

        mov     r2, #(16*2 + 6)
CV1_Loop_Start:
        mov     r3, r5
        ldr     r5, [r0], #4*(1)
        mov     r4, r6
        ldr.w   r6, [r0, #4*(2*64-1)]
        rsb     r14, r4, #0

        smlal   r7, r8, r5, r3
        smlal   r7, r8, r6, r4
        smlal   r9, r10, r3, r6
        smlal   r9, r10, r14, r5
        smlal   r11, r12, r3, r3
        smlal   r11, r12, r4, r4

        subs    r2, r2, #1
        bne     CV1_Loop_Start

        stmia   r1, {r7-r12}

        ldr     r0, [r1, #4*(6)]
        ldr     r2,  [r1, #4*(7)]
        rsb     r3, r3, #0
        adds    r7, r0, r7
        adc     r8, r2,  r8
        smlal   r7, r8, r5, r3
        smlal   r7, r8, r6, r14

        ldr     r0, [r1, #4*(8)]
        ldr     r2,  [r1, #4*(9)]
        adds    r9, r0, r9
        adc     r10, r2,  r10
        smlal   r9, r10, r3, r6
        smlal   r9, r10, r4, r5

        ldr     r0, [r1, #4*(10)]
        ldr     r2,  [r1, #4*(11)]
        adds    r11, r0, r11
        adc     r12, r2, r12
        rsb     r0, r3, #0
        smlal   r11, r12, r3, r0
        rsb     r2,  r4, #0
        smlal   r11, r12, r4, r2

        add     r1, r1, #(4*6)
        stmia   r1, {r7-r12}

    ldmfd   sp!, {r4-r11, pc}
  .size  CVKernel1, .-CVKernel1
//}}}
//{{{  void CVKernel2 (int* XBuf, int* accBuf)
  .section  .text.CVKernel2
  .type  CVKernel2, %function
  .global  CVKernel2
CVKernel2:
        stmfd   sp!, {r4-r11, r14}

        mov     r7, #0
        mov     r8, #0
        mov     r9, #0
        mov     r10, #0

        ldr     r3, [r0], #4*(1)
        ldr.w     r4, [r0, #4*(2*64-1)]
        ldr     r5, [r0], #4*(1)
        ldr.w     r6, [r0, #4*(2*64-1)]

        mov     r2, #(16*2 + 6)
CV2_Loop_Start:
        ldr     r11, [r0], #4*(1)
        ldr.w   r12, [r0, #4*(2*64-1)]
        rsb     r14, r4, #0

        smlal   r7, r8, r11, r3
        smlal   r7, r8, r12, r4
        smlal   r9, r10, r3, r12
        smlal   r9, r10, r14, r11

        mov     r3, r5
        mov     r4, r6
        mov     r5, r11
        mov     r6, r12

        subs    r2, r2, #1
        bne     CV2_Loop_Start

        stmia   r1, {r7-r10}

        ldmfd   sp!, {r4-r11, pc}
  .size  CVKernel2, .-CVKernel2
//}}}
//{{{  void QMFSynthesisConv (int* cPtr, int* delay, int dIdx, short* outbuf, int nChans);
CPTR    .req    r0
DELAY   .req    r1
DIDX    .req    r2
OUTBUF  .req    r3
K       .req    r4
DOFF0   .req    r5
DOFF1   .req    r6
SUMLO   .req    r7
SUMHI   .req    r8
NCHANS  .req    r9
COEF0   .req    r10
DVAL0   .req    r11
COEF1   .req    r12
DVAL1   .req    r14

        .set    FBITS_OUT_QMFS, (14 - (1+2+3+2+1) - (2+3+2) + 6 - 1)
        .set    RND_VAL,    (1 << (FBITS_OUT_QMFS - 1))

  .section  .text.QMFSynthesisConv
  .type  QMFSynthesisConv, %function
  .global  QMFSynthesisConv
QMFSynthesisConv:
        stmfd   sp!, {r4-r11, r14}

        ldr     NCHANS,  [r13, #4*9]    @ we saved 9 registers on stack
        mov     DOFF0, DIDX, lsl #7 @ dOff0 = 128*dIdx
        subs    DOFF1, DOFF0, #1    @ dOff1 = dOff0 - 1
        it      lt
        addlt   DOFF1, DOFF1, #1280 @ if (dOff1 < 0) then dOff1 += 1280
        mov K, #64

S_SRC_Loop_Start:
        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smull   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        it      lt
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        it      lt
        addlt   DOFF1, DOFF1, #1280

        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smlal   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        it      lt
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        it      lt
        addlt   DOFF1, DOFF1, #1280

        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smlal   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        it      lt
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        it      lt
        addlt   DOFF1, DOFF1, #1280

        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smlal   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        it      lt
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        it      lt
        addlt   DOFF1, DOFF1, #1280

        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smlal   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        it      lt
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        it      lt
        addlt   DOFF1, DOFF1, #1280

        add     DOFF0, DOFF0, #1
        sub     DOFF1, DOFF1, #1

        add     SUMHI, SUMHI, #RND_VAL
        mov     SUMHI, SUMHI, asr #FBITS_OUT_QMFS
        mov     SUMLO, SUMHI, asr #31
        cmp     SUMLO, SUMHI, asr #15
        itt     ne
        eorne   SUMHI, SUMLO, #0x7f00   @ takes 2 instructions for immediate value of 0x7fffffff
        eorne   SUMHI, SUMHI, #0x00ff
        strh    SUMHI, [OUTBUF, #0]
        add     OUTBUF, OUTBUF, NCHANS, lsl #1

        subs    K, K, #1
        bne     S_SRC_Loop_Start

        ldmfd   sp!, {r4-r11, pc}
  .size  QMFSynthesisConv, .-QMFSynthesisConv
//}}}
//{{{  void QMFAnalysisConv (int* cTab, int* delay, int dIdx, int* uBuf)
  .section  .text.QMFAnalysisConv
  .type  QMFAnalysisConv, %function
  .global  QMFAnalysisConv
QMFAnalysisConv:
        stmfd   sp!, {r4-r11, r14}

        mov     r6, r2, lsl #5      @ dOff0 = 32*dIdx
        add     r6, r6, #31         @ dOff0 = 32*dIdx + 31
        add     r4, r0, #4*(164)    @ cPtr1 = cPtr0 + 164

        @ special first pass (flip sign for cTab[384], cTab[512])
        ldr     r11, [r0], #4
        ldr     r14, [r0], #4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        smull   r7, r8, r11, r12
        smull   r9, r10, r14, r2

        ldr     r11, [r0], #4
        ldr     r14, [r0], #4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r0], #4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r4], #-4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        rsb     r11, r11, #0
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r4], #-4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        rsb     r11, r11, #0
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        str     r10, [r3, #4*32]
        str     r8, [r3], #4
        sub     r6, r6, #1
        mov     r5, #31

A_SRC_Loop_Start:
        ldr     r11, [r0], #4
        ldr     r14, [r0], #4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        smull   r7, r8, r11, r12
        smull   r9, r10, r14, r2

        ldr     r11, [r0], #4
        ldr     r14, [r0], #4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r0], #4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r4], #-4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r4], #-4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        it      lt
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        str     r10, [r3, #4*32]
        str     r8, [r3], #4
        sub     r6, r6, #1

        subs    r5, r5, #1
        bne     A_SRC_Loop_Start

        ldmfd   sp!, {r4-r11, pc}
  .size  QMFAnalysisConv, .-QMFAnalysisConv
//}}}
