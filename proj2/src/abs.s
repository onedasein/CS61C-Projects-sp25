.globl abs

.text
# =================================================================
# FUNCTION: Given an int return its absolute value.
# Arguments:
#   a0 (int*) is a pointer to the input integer
# Returns:
#   None
# =================================================================
abs:
    # Prologue
    
    # PASTE HERE
    # load number from memory
    lw t0 0(a0)
    bgt t0, zero, done

    # 对 a0 取反
    sub t0, x0, t0

    # store the number in memory
    sw t0 0(a0)

done:
    # Epilogue

    jr ra
