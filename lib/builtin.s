	.text
	.attribute	4, 16
	.attribute	5, "rv32i2p0_m2p0_a2p0"
	.file	"builtin.c"
	.globl	__String_substring__            # -- Begin function __String_substring__
	.p2align	1
	.type	__String_substring__,@function
__String_substring__:                   # @__String_substring__
# %bb.0:
	addi	sp, sp, -16
	sw	ra, 12(sp)                      # 4-byte Folded Spill
	sub	a2, a2, a1      # $a2 = len = r - l
	add	a1, a0, a1      # $a1 = ptr + l
	sw	a2, 8(sp)       # M[$sp + 8] = $a2 = len
	sw	a1, 4(sp)       # M[$sp + 4] = $a1 = ptr + l
	addi	a0, a2, 1   # $a0 = len + 1
	call	malloc
	lw	ra, 12(sp)                      # 4-byte Folded Reload
	lw	a2, 8(sp)       # $a2 = M[$sp + 8] = len
	lw	a1, 4(sp)       # $a1 = M[$sp + 4] = ptr + l
	add	a4, a0, a2      # $a4 = $a0 + len
	sb	zero, 0(a4)     # M[$a0 + len] = 0 ; null-terminate
	addi	sp, sp, 16  # prepare for tail call
	tail	memcpy
.Lfunc_end0:
	.size	__String_substring__, .Lfunc_end0-__String_substring__
                                        # -- End function
	.globl	__String_parseInt__             # -- Begin function __String_parseInt__
	.p2align	1
	.type	__String_parseInt__,@function
__String_parseInt__:                    # @__String_parseInt__
# %bb.0:
	addi	sp, sp, -16
	sw	ra, 12(sp)                      # 4-byte Folded Spill
    la  a1, .L.str
	addi	a2, sp, 8   # $a2 = $sp + 8 = &n
	call	sscanf
	lw	a0, 8(sp)       # $a0 = M[$sp + 8] = n
	lw	ra, 12(sp)                      # 4-byte Folded Reload
	addi	sp, sp, 16
	ret
.Lfunc_end1:
	.size	__String_parseInt__, .Lfunc_end1-__String_parseInt__
                                        # -- End function
	.globl	__String_ord__                  # -- Begin function __String_ord__
	.p2align	1
	.type	__String_ord__,@function
__String_ord__:                         # @__String_ord__
# %bb.0:
	add	a0, a0, a1
	lbu	a0, 0(a0)
	ret
.Lfunc_end2:
	.size	__String_ord__, .Lfunc_end2-__String_ord__
                                        # -- End function
	.globl	__print__                       # -- Begin function __print__
	.p2align	1
	.type	__print__,@function
__print__:                              # @__print__
# %bb.0:
	mv	a1, a0
	lui	a0, %hi(.L.str.1)
	addi	a0, a0, %lo(.L.str.1)
	tail	printf
.Lfunc_end3:
	.size	__print__, .Lfunc_end3-__print__
                                        # -- End function
	.globl	__printInt__                    # -- Begin function __printInt__
	.p2align	1
	.type	__printInt__,@function
__printInt__:                           # @__printInt__
# %bb.0:
	mv	a1, a0
	lui	a0, %hi(.L.str)
	addi	a0, a0, %lo(.L.str)
	tail	printf
.Lfunc_end4:
	.size	__printInt__, .Lfunc_end4-__printInt__
                                        # -- End function
	.globl	__printlnInt__                  # -- Begin function __printlnInt__
	.p2align	1
	.type	__printlnInt__,@function
__printlnInt__:                         # @__printlnInt__
# %bb.0:
	mv	a1, a0
	lui	a0, %hi(.L.str.2)
	addi	a0, a0, %lo(.L.str.2)
	tail	printf
.Lfunc_end5:
	.size	__printlnInt__, .Lfunc_end5-__printlnInt__
                                        # -- End function
	.globl	__getString__                   # -- Begin function __getString__
	.p2align	1
	.type	__getString__,@function
__getString__:                          # @__getString__
# %bb.0:
	addi	sp, sp, -16
	sw	ra, 12(sp)                      # 4-byte Folded Spill
	li	a0, 256
	call	malloc
	sw	a0, 8(sp)
	mv	a1, a0
	lui	a0, %hi(.L.str.1)
	addi	a0, a0, %lo(.L.str.1)
	call	scanf
    lw	a0, 8(sp)
	lw	ra, 12(sp)                      # 4-byte Folded Reload
	addi	sp, sp, 16
	ret
.Lfunc_end6:
	.size	__getString__, .Lfunc_end6-__getString__
                                        # -- End function
	.globl	__getInt__                      # -- Begin function __getInt__
	.p2align	1
	.type	__getInt__,@function
__getInt__:                             # @__getInt__
# %bb.0:
	addi	sp, sp, -16
	sw	ra, 12(sp)                      # 4-byte Folded Spill
	lui	a0, %hi(.L.str)
	addi	a0, a0, %lo(.L.str)
	addi	a1, sp, 8
	call	scanf
	lw	a0, 8(sp)
	lw	ra, 12(sp)                      # 4-byte Folded Reload
	addi	sp, sp, 16
	ret
.Lfunc_end7:
	.size	__getInt__, .Lfunc_end7-__getInt__
                                        # -- End function
	.globl	__toString__                    # -- Begin function __toString__
	.p2align	1
	.type	__toString__,@function
__toString__:                           # @__toString__
# %bb.0:
	addi	sp, sp, -16
	sw	ra, 12(sp)                      # 4-byte Folded Spill
	sw	a0, 8(sp)		# M[$sp + 8] = n
    li  a0, 10          # Make sure the buffer is big enough
	call	malloc
	lw	a2, 8(sp)       # $a2 = M[$sp + 8] = n
    la  a1, .L.str
	sw	a0, 8(sp)		# sp[8] = str
	call	sprintf
	lw	a0, 8(sp)
	lw	ra, 12(sp)                      # 4-byte Folded Reload
	addi	sp, sp, 16
	ret
.Lfunc_end8:
	.size	__toString__, .Lfunc_end8-__toString__
                                        # -- End function
	.globl	__string_add__                  # -- Begin function __string_add__
	.p2align	1
	.type	__string_add__,@function
__string_add__:                         # @__string_add__
# %bb.0:
	addi	sp, sp, -32
	sw	ra, 28(sp)                      # 4-byte Folded Spill
	sw	a0, 24(sp)		# sp[24] = lhs
	sw	s1, 20(sp)                      # 4-byte Folded Spill
	sw	s2, 16(sp)                      # 4-byte Folded Spill
	sw	s0, 12(sp)                      # 4-byte Folded Spill
	mv	s1, a1			# s1 = rhs
	call	strlen
	mv	s2, a0			# s2 = len1
	mv	a0, s1			# a0 = rhs
	call	strlen
	addi	s0, a0, 1	# s0 = len2 + 1
	add	a0, s2, s0		# a0 = len1 + len2 + 1
	call	malloc
	add	a0, a0, s2		# a0 = %malloc + len1
	mv	a1, s1			# a1 = rhs
	mv	a2, s0			# a2 = len2 + 1
	call	memcpy
	sub	a0, a0, s2		# a0 = %malloc
	mv	a2, s2			# a2 = len1
	lw	ra, 28(sp)                      # 4-byte Folded Reload
	lw	a1, 24(sp)		# a1 = lhs
	lw	s1, 20(sp)                      # 4-byte Folded Reload
	lw	s2, 16(sp)                      # 4-byte Folded Reload
	lw	s0, 12(sp)                      # 4-byte Folded Reload
	addi	sp, sp, 32
	tail	memcpy
.Lfunc_end9:
	.size	__string_add__, .Lfunc_end9-__string_add__
                                        # -- End function
	.globl	__Array_size__                  # -- Begin function __Array_size__
	.p2align	1
	.type	__Array_size__,@function
__Array_size__:                         # @__Array_size__
# %bb.0:
	lw	a0, -4(a0)
	ret
.Lfunc_end10:
	.size	__Array_size__, .Lfunc_end10-__Array_size__
                                        # -- End function
	.globl	__new_array4__                  # -- Begin function __new_array4__
	.p2align	1
	.type	__new_array4__,@function
__new_array4__:                         # @__new_array4__
# %bb.0:
	addi	sp, sp, -16
	sw	ra, 12(sp)                      # 4-byte Folded Spill
	sw	s0, 8(sp)                       # 4-byte Folded Spill
	mv	s0, a0
	slli	a0, a0, 2
	addi	a0, a0, 4
	call	malloc
	addi	a1, a0, 4
	sw	s0, 0(a0)
	mv	a0, a1
	lw	ra, 12(sp)                      # 4-byte Folded Reload
	lw	s0, 8(sp)                       # 4-byte Folded Reload
	addi	sp, sp, 16
	ret
.Lfunc_end11:
	.size	__new_array4__, .Lfunc_end11-__new_array4__
                                        # -- End function
	.globl	__new_array1__                  # -- Begin function __new_array1__
	.p2align	1
	.type	__new_array1__,@function
__new_array1__:                         # @__new_array1__
# %bb.0:
	addi	sp, sp, -16
	sw	ra, 12(sp)                      # 4-byte Folded Spill
	sw	s0, 8(sp)                       # 4-byte Folded Spill
	mv	s0, a0
	addi	a0, a0, 4
	call	malloc
	addi	a1, a0, 4
	sw	s0, 0(a0)
	mv	a0, a1
	lw	ra, 12(sp)                      # 4-byte Folded Reload
	lw	s0, 8(sp)                       # 4-byte Folded Reload
	addi	sp, sp, 16
	ret
.Lfunc_end12:
	.size	__new_array1__, .Lfunc_end12-__new_array1__
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"%d"
	.size	.L.str, 3

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"%s"
	.size	.L.str.1, 3

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"%d\n"
	.size	.L.str.2, 4

	.ident	"Ubuntu clang version 15.0.7"
	.section	".note.GNU-stack","",@progbits
