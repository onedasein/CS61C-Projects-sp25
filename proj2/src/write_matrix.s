.globl write_matrix

.text
# ==============================================================================
# FUNCTION: Writes a matrix of integers into a binary file
# FILE FORMAT:
#   The first 8 bytes of the file will be two 4 byte ints representing the
#   numbers of rows and columns respectively. Every 4 bytes thereafter is an
#   element of the matrix in row-major order.
# Arguments:
#   a0 (char*) is the pointer to string representing the filename
#   a1 (int*)  is the pointer to the start of the matrix in memory
#   a2 (int)   is the number of rows in the matrix
#   a3 (int)   is the number of columns in the matrix
# Returns:
#   None
# Exceptions:
#   - If you receive an fopen error or eof,
#     this function terminates the program with error code 27
#   - If you receive an fclose error or eof,
#     this function terminates the program with error code 28
#   - If you receive an fwrite error or eof,
#     this function terminates the program with error code 30
# ==============================================================================
write_matrix:

    # Prologue
    addi sp, sp, -24
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    sw s2, 12(sp)
    sw s3, 16(sp)
    sw s4, 20(sp)

    mv s0, a1
    mv s2, a2
    mv s3, a3
    # open file
    li a1, 1
    jal fopen
    li t0, -1
    beq a0, t0, fopen_error
    mv s1, a0
    
    # write rows and columns
    addi sp, sp, -8
    sw s2, 0(sp)
    sw s3, 4(sp)
    mv a0, s1
    mv a1, sp
    li a2, 2
    li a3, 4
    jal fwrite
    li t0, 2
    bne a0, t0, fwrite_error
    lw s2, 0(sp)
    lw s3, 4(sp)
    addi sp, sp, 8
    # write matrix
    mv a0, s1
    mv a1, s0
    mul a2, s2, s3
    mv s4, a2
    li a3, 4
    jal fwrite
    bne a0, s4, fwrite_error

    mv a0, s1
    jal fclose
    li t0, -1
    beq a0, t0, fclose_error

    # Epilogue
    mv a1, s0
    lw ra, 0(sp)
    lw s0, 4(sp)
    lw s1, 8(sp)
    lw s2, 12(sp)
    lw s3, 16(sp)
    lw s4, 20(sp)
    addi sp, sp, 24
    jr ra
fopen_error:
    li a0, 27
    j exit
fwrite_error:
    li a0, 30
    j exit
fclose_error:
    li a0, 28
    j exit

