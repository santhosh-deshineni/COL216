# Define main as the global label
		.text
		.globl main



main:	addi $sp,$sp,-4
		addi $s1,$zero,4
		sw $s1,0($sp)
		sll $t0,$s0,2
		sub $sp,$sp,$t0
		addi $s0,$zero,2
		sw $s0,0($sp)
		addi $s0,$zero,3
		sw $s0,4($sp)
		addi $s0,$zero,3
		sw $s0,8($sp)
		addi $s0,$zero,3
		sw $s0,12($sp)
		addi $t0,$zero,0
		addi $t1,$s1,-1
	
Loop:	slt $t3,$t0,$t1
		beq $t3,0,Exit
		add $t2,$t0,$t1
		addi $t2,$t2,1
		srl $t2,$t2,1
		sll $t4,$t2,2
		add $t4,$sp,$t4
		lw $t5,0($t4)
		bne $t5,$s0,Elseif
		add $v0,$zero,$t2
		j Exit
	
Elseif:	slt $t6,$t5,$s0
		beq $t6,0,Else
		addi $t0,$t2,1
		j Loop

Else:	addi $t1,$t2,-1
		j Loop
Exit:	jr $ra
	
	