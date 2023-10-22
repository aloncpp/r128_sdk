/*void set_fpu_register_test(int s0, void *data, int count, int reserved);*/
.global set_fpu_register_test

set_fpu_register_test:
	vmov s0, r0
	vmov s1, #1
	vmov s2, #2
	vmov s3, #3
	vmov s4, #4
	vmov s5, #5
	vmov s6, #6
	vmov s7, #7
	vmov s8, #8
	vmov s9, #9
	vmov s10, #10
	vmov s11, #11
	vmov s12, #12
	vmov s13, #13
	vmov s14, #14
	vmov s15, #15
loop:
	sub r2, r2, #1
	cmp  r2, #0
	bne  loop

	vstr s0, [r1, #0]
	vstr s1, [r1, #4]
	vstr s2, [r1, #8]
	vstr s3, [r1, #12]
	vstr s4, [r1, #16]
	vstr s5, [r1, #20]
	vstr s6, [r1, #24]
	vstr s7, [r1, #28]
	vstr s8, [r1, #32]
	vstr s9, [r1, #36]
	vstr s10, [r1, #40]
	vstr s11, [r1, #44]
	vstr s12, [r1, #48]
	vstr s13, [r1, #52]
	vstr s14, [r1, #56]
	vstr s15, [r1, #60]
	bx lr

