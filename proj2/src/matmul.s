.globl matmul

.text
# =======================================================
# FUNCTION: Matrix Multiplication of 2 integer matrices
#   d = matmul(m0, m1)
# Arguments:
#   a0 (int*)  is the pointer to the start of m0
#   a1 (int)   is the # of rows (height) of m0
#   a2 (int)   is the # of columns (width) of m0
#   a3 (int*)  is the pointer to the start of m1
#   a4 (int)   is the # of rows (height) of m1
#   a5 (int)   is the # of columns (width) of m1
#   a6 (int*)  is the pointer to the the start of d
# Returns:
#   None (void), sets d = matmul(m0, m1)
# Exceptions:
#   Make sure to check in top to bottom order!
#   - If the dimensions of m0 do not make sense,
#     this function terminates the program with exit code 38
#   - If the dimensions of m1 do not make sense,
#     this function terminates the program with exit code 38
#   - If the dimensions of m0 and m1 don't match,
#     this function terminates the program with exit code 38
# =======================================================
matmul:

    # Error checks
    li t0, 1
    blt a1, t0, error_exit
    blt a2, t0, error_exit
    blt a4, t0, error_exit
    blt a5, t0, error_exit
    bne a2, a4, error_exit

    # Prologue
    addi sp, sp, -28
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    sw s2, 12(sp)
    sw s3, 16(sp)
    sw s4, 20(sp)
    sw s5, 24(sp)
    # load
    mv s0, a0                    # s0 = a0
    mv s1, a3                    # s1 = a3
    mv s2, a6                    # s2 = a6(third-address)
    mv s3, a2                    # s3 = a2(first-width)
    mv s4, a1                    # s4 = a1(first-height)
    mv s5, a5                    # s5 = a5(second-width)

    li t0, 0
outer_loop_start:
    beq t0, s4, outer_loop_end
    li t1, 0

inner_loop_start:
    beq t1, s5, inner_loop_end
    # a0 is address of the first array
    mv t2, t0
    mul t2, t2, s3
    slli t2, t2, 2
    add a0, s0, t2
    # a1 is address of the second array
    mv t2, t1
    slli t2, t2, 2
    add a1, s1, t2
    # a2 is fixed number m
    mv a2, s3 
    li a3, 1
    mv a4, s5
    
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    jal dot
    lw t0, 0(sp)
    lw t1, 4(sp)
    addi sp, sp, 8

    mv t2, t0               # t2 = i
    mul t2, t2, s5          # t2 = i * k
    add t2, t2, t1          # t2 = i * m + j
    slli t2, t2, 2          # offset
    add t5, s2, t2
    sw a0, 0(t5)

    addi t1, t1, 1
    j inner_loop_start

inner_loop_end:
    addi t0, t0, 1
    j outer_loop_start

outer_loop_end:
    # Epilogue
    lw ra, 0(sp)
    lw s0, 4(sp)
    lw s1, 8(sp)
    lw s2, 12(sp)
    lw s3, 16(sp)
    lw s4, 20(sp)
    lw s5, 24(sp)
    addi sp, sp, 28
    jr ra

error_exit:
    li a0, 38
    j exit
