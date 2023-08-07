# Define the prompts and set main as the global label
        .data
prompt1: 	.asciiz "Enter x: "
prompt2: 	.asciiz "Enter n: "

		.text
		.globl main

# The input format is to take x in the first line followed by n in second line

main:   # Print the first prompt
        li $v0, 4
		la $a0, prompt1
		syscall
        # Take x from input
		li $v0, 5
		syscall
        move $t0, $v0
        # Print the second prompt
		li $v0 , 4
		la $a0, prompt2
		syscall
        # Take n from input
		li $v0, 5
		syscall
        # Place x and n in $a0 and $a1 respectively
        move $a1, $v0
        move $a0, $t0
        # Jump to power after linking
		jal power
		j out

power:  # Check if the current n is zero to decide on jump
        bne $a1,$zero,Elseif
        # If it is zero then place 1 in $v0 or output
		addi $v0,$zero,1
        # Jump to return address
		jr $ra

Elseif:	# Shift right and left to make LSB zero
        # Now compare with original number to check if number is even
        srl $t0,$a1,1
		sll $t1,$t0,1
		beq $t1,$a1,Else
        # n is odd
        # Store return address and decrement n by 1
		addi $sp,$sp,-4
		addi $a1,$a1,-1
		sw $ra,0($sp)
        # Jump back to power with decremented n
		jal power
        # Multiply output of power with (x) and (n-1) as inputs with x
        # This gives us x^(n)
		mult $a0,$v0
		mflo $v0
		lw $ra,0($sp)
		addi $sp,$sp,4
        # Jump to return address
		jr $ra

Else:	# n is even
        # Store return address and make n to half of current value
        add $a1,$t0,$zero
		addi $sp,$sp,-4
		sw $ra,0($sp)
        # Jump back to power with halved n
		jal power
        # Multiply output of power with (x) and (n/2) as inputs with itself
        # this gives us x^(n)
		mult $v0,$v0
		mflo $v0
		lw $ra,0($sp)
		addi $sp,$sp,4
        # Jump to return address
		jr $ra

out:    # Print the output that is x^(n)
        move $a0, $v0
		li $v0, 1
		syscall
        # Exit the program
		li $v0, 10
        syscall