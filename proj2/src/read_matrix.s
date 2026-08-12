.globl read_matrix

.text
# ==============================================================================
# FUNCTION: Allocates memory and reads in a binary file as a matrix of integers
#
# FILE FORMAT:
#   The first 8 bytes are two 4 byte ints representing the # of rows and columns
#   in the matrix. Every 4 bytes afterwards is an element of the matrix in
#   row-major order.
# Arguments:
#   a0 (char*) is the pointer to string representing the filename
#   a1 (int*)  is a pointer to an integer, we will set it to the number of rows
#   a2 (int*)  is a pointer to an integer, we will set it to the number of columns
# Returns:
#   a0 (int*)  is the pointer to the matrix in memory
# Exceptions:
#   - If malloc returns an error,
#     this function terminates the program with error code 26
#   - If you receive an fopen error or eof,
#     this function terminates the program with error code 27
#   - If you receive an fclose error or eof,
#     this function terminates the program with error code 28
#   - If you receive an fread error or eof,
#     this function terminates the program with error code 29
# ==============================================================================
read_matrix:

    # Prologue
    addi sp, sp, -28
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    sw s2, 12(sp)
    sw s3, 16(sp)
    sw s4, 20(sp)
    sw s5, 24(sp)

    mv s4, a1
    mv s5, a2

    li a1, 0
    jal fopen
    li t0, -1
    beq a0, t0, fopen_error
    mv s0, a0
    
    # read rows and cols
    addi sp, sp, -8
    mv a1, sp
    li s2, 8
    mv a2, s2
    jal fread
    bne a0, s2, fread_error
    
    # malloc
    lw t0, 0(sp)
    lw t1, 4(sp)
    mul a2, t0, t1
    slli a2, a2, 2
    mv s3, a2
    mv a0, a2
    jal malloc
    beq a0, x0, malloc_error
    mv s1, a0
    
    # read matrix
    mv a0, s0
    mv a1, s1
    mv a2, s3
    jal fread
    bne a0, s3, fread_error

    # close file
    mv a0, s0
    jal fclose
    li t0, -1
    beq a0, t0, fclose_error

    # Epilogue
    mv a0, s1
    lw t0, 0(sp)
    sw t0, 0(s4)
    lw t0, 4(sp)
    sw t0, 0(s5)
    addi sp, sp, 8
    lw ra, 0(sp)
    lw s0, 4(sp)
    lw s1, 8(sp)
    lw s2, 12(sp)
    lw s3, 16(sp)
    lw s4, 20(sp)
    lw s5, 24(sp)
    addi sp, sp, 28

    jr ra
malloc_error:
    li a0, 26
    j exit
fopen_error:
    li a0, 27
    j exit
fclose_error:
    li a0, 28
    j exit
fread_error:
    li a0, 29
    j exit
