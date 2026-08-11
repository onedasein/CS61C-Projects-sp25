.globl dot

.text
# =======================================================
# FUNCTION: Dot product of 2 int arrays
# Arguments:
#   a0 (int*) is the pointer to the start of arr0
#   a1 (int*) is the pointer to the start of arr1
#   a2 (int)  is the number of elements to use
#   a3 (int)  is the stride of arr0
#   a4 (int)  is the stride of arr1
# Returns:
#   a0 (int)  is the dot product of arr0 and arr1
# Exceptions:
#   - If the number of elements to use is less than 1,
#     this function terminates the program with error code 36
#   - If the stride of either array is less than 1,
#     this function terminates the program with error code 37
# =======================================================
dot:

    # Prologue
    li t0, 1
    blt a2, t0, number_error
    blt a3, t0, stride_error
    blt a4, t0, stride_error
    li t0, 0
    li t4, 0
loop_start:
    beq t0, a2, loop_end
    mul t1, t0, a3         # t1 = t0 * a3 
    mul t2, t0, a4         # t2 = t0 * a4
    slli t1, t1, 2
    slli t2, t2, 2
    add t1, a0, t1         # t1 = a0 + t1
    add t2, a1, t2         # t2 = a0 + t2
    lw t1, 0(t1)           # t1 = M[R[t1]]
    lw t2, 0(t2)           # t2 = M[R[t2]]
    mul t3, t1, t2         # t3 = t1 * t2
    add t4, t4, t3         # t4 = t4 + t3
    addi t0, t0, 1         # t0++
    j loop_start
loop_end:
    mv a0, t4              # a0 = sum = t4
    # Epilogue
    jr ra
number_error:
    li a0, 36
    j exit
stride_error:
    li a0, 37
    j exit
