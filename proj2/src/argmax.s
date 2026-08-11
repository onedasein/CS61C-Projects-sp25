.globl argmax

.text
# =================================================================
# FUNCTION: Given a int array, return the index of the largest
#   element. If there are multiple, return the one
#   with the smallest index.
# Arguments:
#   a0 (int*) is the pointer to the start of the array
#   a1 (int)  is the # of elements in the array
# Returns:
#   a0 (int)  is the first index of the largest element
# Exceptions:
#   - If the length of the array is less than 1,
#     this function terminates the program with error code 36
# =================================================================
argmax:
    # Prologue
    li t0, 1
    blt a1, t0, error_exit
    li t0, 1
    lw t1, 0(a0)
    li t5, 0
loop_start:
    beq t0, a1, loop_end
    slli t2, t0, 2       # offset = t0 * 4
    add t3, a0, t2
    lw t4, 0(t3)
    ble t4, t1, loop_continue
    mv t5, t0
    mv t1, t4

loop_continue:
    addi t0, t0, 1
    j loop_start
loop_end:
    # Epilogue
    mv a0, t5
    jr ra
error_exit:
    li a0, 36
    j exit
