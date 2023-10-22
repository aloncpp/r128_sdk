#define frame   r4
#define sv_fp   r5
#define sv_pc   r6
#define mask    r7
#define offset  r8

.global __backtrace

__backtrace:
    cmp r0, #0
    moveq r0, fp

#    mov r1, #0x10
#    mov r0, fp

    stmfd   sp!, {r1 - r9, lr}  @ Save an extra register so we have a location...
    movs    frame, r0           @ if frame pointer is zero
    beq no_frame                @ we have no stack frames

    mov r1, #0x10
    tst r1, #0x10               @ 26 or 32-bit mode?
    moveq   mask, #0xfc000003   @ mask for 26-bit
    movne   mask, #0            @ mask for 32-bit

1:  stmfd   sp!, {pc}           @ calculate offset of PC stored
    ldr r0, [sp], #4            @ by stmfd for this CPU
    adr r1, 1b
    sub offset, r0, r1

/*
 * Stack frame layout:
 *         optionally saved caller registers (r4 - r10)
 *         saved fp
 *         saved sp
 *         saved lr
 *    frame => saved pc
 *         optionally saved arguments (r0 - r3)
 * saved sp => <next word>
 *
 * Functions start with the following code sequence:
 *          mov   ip, sp
 *          stmfd sp!, {r0 - r3} (optional)
 * corrected pc =>  stmfd sp!, {..., fp, ip, lr, pc}
 */
for_each_frame: tst frame, mask     @ Check for address exceptions
    bne no_frame

    ldr r0,=0x11111111
    teq frame, #0               @ zero saved fp means
    ldreq r0,=0x0
    beq no_frame                @ no further frames
    teq frame, r0               @ zero saved fp means
    ldreq r0,=0x0
    beq no_frame                @ no further frames

    mov r0, frame
    ldmfd   sp!, {r1-r3}        @ get mute value
    stmfd   sp!, {r1-r3}
    mov r1, r3
    blx check_frame_corrupted
    teq r0, #0
    ldreq r0,=0x0
    beq no_frame

1001:
    ldr sv_pc, [frame, #0]      @ get saved pc
1002:
    ldr sv_fp, [frame, #-12]    @ get saved fp

    sub sv_pc, sv_pc, offset    @ Correct PC for prefetching
    bic sv_pc, sv_pc, mask      @ mask PC/LR for the mode

1003:
    ldr r2, [sv_pc, #-4]        @ if stmfd sp!, {args} exists,
    ldr r3, .Ldsi+4             @ adjust saved 'pc' back one
    teq r3, r2, lsr #10         @ instruction
    subne   r0, sv_pc, #4       @ allow for mov
    subeq   r0, sv_pc, #8       @ allow for mov + stmia

    ldr r1, [frame, #-4]        @ get saved lr
    #mov r2, frame
    bic r1, r1, mask            @ mask PC/LR for the mode
    mov r9, r0
    ldmfd   sp!, {r3}           @ backtrace_entry
    ldmfd   sp!, {r2}           @ level
    sub r2, r2, #1
    stmfd   sp!, {r2}
    stmfd   sp!, {r3}
    add r2, r2, #1
    bl  backtrace_hook

    teq r0, #0
    ldreq r0, [frame, #-4]      @ get saved lr
    beq no_frame

    ldr r0, =vApplicationFPUSafeIRQHandler
    teq r9, r0
    moveq r0, r0
    beq no_frame
    teq sv_fp, #0               @ zero saved fp means
    ldreq r0,=0x0
    beq no_frame                @ no further frames
    teq sv_fp, r0               @ zero saved fp means
    moveq r0, r0
    beq no_frame                @ no further frames

    cmp sv_fp, frame            @ next frame must be
    mov frame, sv_fp            @ above the current frame
    b   for_each_frame

no_frame:
    ldmfd   sp!, {r1 - r9, pc}

    .align 4
.Ldsi:      .word   0xe92dd800 >> 10    @ stmfd sp!, {... fp, ip, lr, pc}
    .word   0xe92d0000 >> 10    @ stmfd sp!, {}
