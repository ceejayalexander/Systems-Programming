! Spring 2022 Revisions by Andrej Vrtanoski

! This program executes pow as a test program using the LC 22 calling convention
! Check your registers ($v0) and memory to see if it is consistent with this program

! vector table
vector0:
        .fill 0x00000000                        ! device ID 0
        .fill 0x00000000                        ! device ID 1
        .fill 0x00000000                        ! ...
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000                        ! device ID 7
        ! end vector table

main:	lea $sp, initsp                         ! initialize the stack pointer
        lw $sp, 0($sp)                          ! finish initialization

        lea $t0, vector0                        ! DONE: Install timer interrupt handler into vector table
        lea $t1, timer_handler
        sw  $t1, 0($t0)

        lea $t1, toaster_handler                ! DONE: Install toaster interrupt handler into vector table
        sw  $t1, 1($t0)

        lea $t0, minval
        lw $t0, 0($t0)
        addi $t1, $zero, 65534                  ! store 0000ffff into minval (to make comparisons easier)
        sw $t1, 0($t0)

        ei                                      ! Enable interrupts

        lea $a0, BASE                           ! load base for pow
        lw $a0, 0($a0)
        lea $a1, EXP                            ! load power for pow
        lw $a1, 0($a1)
        lea $at, POW                            ! load address of pow
        jalr $ra, $at                           ! run pow
        lea $a0, ANS                            ! load base for pow
        sw $v0, 0($a0)

        halt                                    ! stop the program here
        addi $v0, $zero, -1                     ! load a bad value on failure to halt

BASE:   .fill 2
EXP:    .fill 8
ANS:	.fill 0                                 ! should come out to 256 (BASE^EXP)

POW:    addi $sp, $sp, -1                       ! allocate space for old frame pointer
        sw $fp, 0($sp)

        addi $fp, $sp, 0                        ! set new frame pointer

        bgt $a1, $zero, BASECHK                 ! check if $a1 is zero
        br RET1                                 ! if the exponent is 0, return 1

BASECHK:bgt $a0, $zero, WORK                    ! if the base is 0, return 0
        br RET0

WORK:   addi $a1, $a1, -1                        ! decrement the power
        lea $at, POW                            ! load the address of POW
        addi $sp, $sp, -2                       ! push 2 slots onto the stack
        sw $ra, -1($fp)                         ! save RA to stack
        sw $a0, -2($fp)                         ! save arg 0 to stack
        jalr $ra, $at                           ! recursively call POW
        add $a1, $v0, $zero                     ! store return value in arg 1
        lw $a0, -2($fp)                         ! load the base into arg 0
        lea $at, MULT                           ! load the address of MULT
        jalr $ra, $at                           ! multiply arg 0 (base) and arg 1 (running product)
        lw $ra, -1($fp)                         ! load RA from the stack
        addi $sp, $sp, 2

        br FIN                                  ! unconditional branch to FIN

RET1:   add $v0, $zero, $zero                   ! return a value of 0
	addi $v0, $v0, 1                        ! increment and return 1
        br FIN                                  ! unconditional branch to FIN

RET0:   add $v0, $zero, $zero                   ! return a value of 0

FIN:	lw $fp, 0($fp)                          ! restore old frame pointer
        addi $sp, $sp, 1                        ! pop off the stack
        jalr $zero, $ra

MULT:   add $v0, $zero, $zero                   ! return value = 0
        addi $t0, $zero, 0                      ! sentinel = 0
AGAIN:  add $v0, $v0, $a0                       ! return value += argument0
        addi $t0, $t0, 1                        ! increment sentinel
        blt $t0, $a1, AGAIN                     ! while sentinel < argument, loop again
        jalr $zero, $ra                         ! return from mult

timer_handler:
        addi $sp, $sp, -1                       ! push 1 slot onto the stack
        sw $k0, 0($sp)                          ! save $k0 on the stack
        ei                                      ! enable interrupts
        addi $sp, $sp, -13                      ! push 13 slots onto the stack
        sw $at, 12($sp)                         ! save at on stack
        sw $v0, 11($sp)                         ! save v0 on stack
        sw $a0, 10($sp)                         ! save a0 on stack
        sw $a1, 9($sp)                          ! save a1 on stack
        sw $a2, 8($sp)                          ! save a2 on stack
        sw $t0, 7($sp)                          ! save t0 on stack
        sw $t1, 6($sp)                          ! save t1 on stack
        sw $t2, 5($sp)                          ! save t2 on stack
        sw $s0, 4($sp)                          ! save s0 on stack
        sw $s1, 3($sp)                          ! save s1 on stack
        sw $s2, 2($sp)                          ! save s2 on stack
        sw $fp, 1($sp)                          ! save fp on stack
        sw $ra, 0($sp)                          ! save ra on stack
        
        lea $t0, ticks                          ! t0 = mem: 0xticks
        lw $t0, 0($t0)                          ! t0 = mem[0xticks] = 0xFFFF
        lw $t1, 0($t0)                          ! t1 = mem[0xFFFF]
        addi $t1, $t1, 1                        ! t1 += 1
        sw $t1, 0($t0)                          ! mem[0xFFFF] = ++t1

        lw $ra, 0($sp)                          ! restore ra
        lw $fp, 1($sp)                          ! restore fp
        lw $s2, 2($sp)                          ! restore s2
        lw $s1, 3($sp)                          ! restore s1
        lw $s0, 4($sp)                          ! restore s0
        lw $t2, 5($sp)                          ! restore t2
        lw $t1, 6($sp)                          ! restore t1
        lw $t0, 7($sp)                          ! restore t0
        lw $a2, 8($sp)                          ! restore a2
        lw $a1, 9($sp)                          ! restore a1
        lw $a0, 10($sp)                         ! restore a0
        lw $v0, 11($sp)                         ! restore v0
        lw $at, 12($sp)                         ! restore at
        addi $sp, $sp, 13                       ! pop 13 slots off the stack
        di                                      ! disable interrupts
        lw $k0, 0($sp)                          ! restore $k0
        addi $sp, $sp, 1                        ! pop $k0 off the stack
        reti

toaster_handler:
        ! retrieve the data from the device and check if it is a minimum or maximum value
        ! then calculate the difference between minimum and maximum value
        ! (hint: think about what ALU operations you could use to implement subract using 2s compliment)

        add $zero, $zero, $zero                 ! TODO FIX ME


initsp: .fill 0xA000
ticks:  .fill 0xFFFF
range:  .fill 0xFFFE
maxval: .fill 0xFFFD
minval: .fill 0xFFFC