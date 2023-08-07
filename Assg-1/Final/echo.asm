# This program takes an input of max 100 characters and prints back the same string
# The input string is stored in the heap
# We use syscalls for i/o and heap memory allocation

# Define the prompts and the set main as global label
        .data
prpt:	.asciiz "Enter a string: "

	.text
	.globl main

main:	# Print the prompt asking for a string
        li $v0, 4
        la $a0, prpt
        syscall

        # Allocate 101 bytes on heap to ensure 100 bytes for characters and last for '\0'
        li $v0, 9
        la $a0, 101
        syscall

        # Read the string from user and store it in the heap
        # Put a max limit of 100 characters on the input string
        # The first byte of the address which is returned by syscall above is used below
        move $t0, $v0
        li $v0, 8
        move $a0, $t0
        li $a1, 100
        syscall

        # Print the string
        li $v0, 4
        syscall

        # Exit the program
        li $v0, 10
        syscall