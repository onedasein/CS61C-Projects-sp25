.globl classify

.text
# =====================================
# COMMAND LINE ARGUMENTS
# =====================================
# Args:
#   a0 (int)        argc
#   a1 (char**)     argv
#   a1[1] (char*)   pointer to the filepath string of m0
#   a1[2] (char*)   pointer to the filepath string of m1
#   a1[3] (char*)   pointer to the filepath string of input matrix
#   a1[4] (char*)   pointer to the filepath string of output file
#   a2 (int)        silent mode, if this is 1, you should not print
#                   anything. Otherwise, you should print the
#                   classification and a newline.
# Returns:
#   a0 (int)        Classification
# Exceptions:
#   - If there are an incorrect number of command line args,
#     this function terminates the program with exit code 31
#   - If malloc fails, this function terminates the program with exit code 26
#
# Usage:
#   main.s <M0_PATH> <M1_PATH> <INPUT_PATH> <OUTPUT_PATH>
classify:
    li t0, 5
    bne a0, t0, argc_error
    addi sp, sp, -52
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    sw s2, 12(sp)
    sw s3, 16(sp)
    sw s4, 20(sp)
    sw s5, 24(sp)
    sw s6, 28(sp)
    sw s7, 32(sp)
    sw s8, 36(sp)
    sw s9, 40(sp)
    sw s10, 44(sp)
    sw s11, 48(sp)
    mv s0, a1
    mv s11, a2
    addi sp, sp, -8
    # Read pretrained m0
    lw a0, 4(s0)
    mv a1, sp
    addi a2, sp, 4
    jal read_matrix
    mv s1, a0
    lw s4, 0(sp)
    lw s5, 4(sp)


    # Read pretrained m1
    lw a0, 8(s0)
    mv a1, sp
    addi a2, sp, 4
    jal read_matrix
    mv s2, a0
    lw s6, 0(sp)
    lw s7, 4(sp)

    # Read input matrix
    lw a0, 12(s0)
    mv a1, sp
    addi a2, sp, 4
    jal read_matrix
    mv s3, a0
    lw t0, 0(sp)
    lw t1, 4(sp)

    # Compute h = matmul(m0, input)
    mv t2, s4
    mv t3, t1
    mul a0, t2, t3
    slli a0, a0, 2
    jal malloc
    beq a0, x0, malloc_error
    mv s8, a0

    mv a0, s1
    mv a1, s4
    mv a2, s5
    mv a3, s3
    lw a4, 0(sp)
    lw a5, 4(sp)
    mv a6, s8
    jal matmul


    # Compute h = relu(h)
    mv a0, s8
    mv t2, s4
    lw t3, 4(sp)
    mul a1, t2, t3
    jal relu
    
    # Compute o = matmul(m1, h)
    mv t2, s6
    lw t3, 4(sp)
    mul a0, t2, t3
    slli a0, a0, 2
    jal malloc
    beq a0, x0, malloc_error
    mv s9, a0

    mv a0, s2
    mv a1, s6
    mv a2, s7
    mv a3, s8
    mv a4, s4
    lw a5, 4(sp)
    mv a6, s9
    jal matmul

    # Write output matrix o
    lw a0, 16(s0)
    mv a1, s9
    mv a2, s6
    lw a3, 4(sp)
    jal write_matrix

    # Compute and return argmax(o)
    mv a0, s9
    mv t0, s6
    lw t1, 4(sp)
    mul a1, t0, t1
    jal argmax
    mv s10, a0

    # If enabled, print argmax(o) and newline
    bne s11, x0, end
    mv a0, s10
    jal print_int
    li a0, '\n'
    jal print_char
end:
    mv a0, s9
    jal free
    mv a0, s8
    jal free
    mv a0, s10
    addi sp, sp, 8
    lw ra, 0(sp)
    lw s0, 4(sp)
    lw s1, 8(sp)
    lw s2, 12(sp)
    lw s3, 16(sp)
    lw s4, 20(sp)
    lw s5, 24(sp)
    lw s6, 28(sp)
    lw s7, 32(sp)
    lw s8, 36(sp)
    lw s9, 40(sp)
    lw s10, 44(sp)
    lw s11, 48(sp)
    addi sp, sp, 52

    jr ra
malloc_error:
    li a0, 26
    j exit
argc_error:
    li a0, 31
    j exit
