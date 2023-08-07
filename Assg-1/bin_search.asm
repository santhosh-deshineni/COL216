# Define the prompts and set main as the global label
		.data
prpt1:	.asciiz "Enter n: "
prpt2:	.asciiz "Enter array: "
prpt3:	.asciiz "Enter x: "
prpt4:	.asciiz "Not Found"
prpt5:	.asciiz "Yes at index "
	
		.text
		.globl main

# The input format is to take one number in each line
# We first take n followed by the array one number per line and finally take x

main:	# Print the first prompt
		li $v0, 4
		la $a0, prpt1
		syscall
		# Read n from the input
		li $v0, 5
		syscall
		# Place n in $s0
		move $s0, $v0
		sll $t0, $s0, 2
		# Allocate space for array that is 4*n bytes on the heap
		li $v0, 9
		move $a0, $t0 
		syscall
		# Place the address of first byte of allocated space in $s1
		move $s1, $v0
		move $t1, $s1
		add $t0, $zero, $zero
		# Print the second prompt
		li $v0, 4
		la $a0, prpt2
		syscall
		# Jump to loop label
		j loop

loop:	# Take integer which is to be added to array
		li $v0, 5
		syscall
		sw $v0, 0($t1)
		# Increment iteration count present in $t0
		# Increment $t1 by 4 to represent address to place next integer
		addi $t1, $t1, 4
		addi $t0, $t0, 1
		# Loop back to take next integer of array if not complete
		bne $s0, $t0, loop
		# Print the third prompt
		li $v0, 4
		la $a0, prpt3
		syscall
		# Take x from the input
		li $v0, 5
		syscall
		# Place x in $s2
		move $s2, $v0
		# Set $s3 which represents i to zero
		add $s3, $zero, $zero
		# Set $s4 which represents j to n-1
		addi $s4, $s0, -1
		# Jump to loop1 label
		j loop1

loop1:	# Jump to notf if (i > j)
		slt $t2, $s4, $s3
		bne $t2, $zero, notf
		# place mid = (i+j+1)//2 in $s5
		add $t3, $s3, $s4
		addi $t3, $t3, 1
		srl $s5, $t3, 1
		sll $t3, $s5, 2
		add $t3, $s1, $t3
		lw $t4, 0($t3)
		# Check equality of a[mid] and x
		bne $t4, $s2, elseif
		j found

elseif:	# If a[mid] < x then we do not jump to else and make i = mid + 1
		# If a[mid] > x then we jump to else
		# Note that they cannot be equal as we already checked the same in loop1
		slt $t5, $t4, $s2
		beq $t5, $zero, else
		addi $s3, $s5, 1
		j loop1

else:	# Make j = mid - 1
		addi $s4, $s5, -1
		j loop1

notf: 	# print the fourth prompt and jump to exit
		li $v0, 4
		la $a0, prpt4
		syscall
		j exit

found:	# print the fifth prompt followed by the corresponding index
		li $v0, 4
		la $a0, prpt5
		syscall
		li $v0, 1
		move $a0, $s5
		syscall
		j exit

exit: 	# Exit the program
		li $v0, 10
		syscall