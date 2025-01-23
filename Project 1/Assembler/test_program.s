;; Sample LC-2200 Assembly Program
;; This program tests arithmetic, memory operations, branching, and halting.
;;"$zero",
;;"$at",
;;"$v0",
;;"$a0",
;;"$a1",
;;"$a2",
;;"$t0",
;;"$t1",
;;"$t2",
;;"$s0",
;;"$s1",
;;"$s2",
;;"$k0",
;;"$sp",
;;"$pr",
;;"$ra"
;
;start:  add     $t0, $zero, $zero    ; $t0 = 0
;        addi    $t1, $t0, 10         ; $t1 = 10
;        addi    $t2, $t1, -5         ; $t2 = 5
;        add     $s0, $t1, $t2        ; $s0 = 15
;        lw      $s1, array($zero)    ; s1 = 20
;        add     $k0, $s0, $s1        ; k0 = 35
;        sw      $k0, result($zero)   ; mem[result] = 35
;        nand    $a0, $t1, $t2        ; 5 NAND 10 = -1 (decimal) and FFFF in hex
;        beq     $zero, $zero, branch ; Unconditionally branch to 'branch'
;here:   add     $a1, $t1, $s1        ; a1 = 30
;        beq     $zero, $zero, done   ; Unconditionally branch to 'done'
;branch: add     $a1, $t1, $k0        ; a1 = 45
;        addi    $a2, $zero, 0x000e      ; a2 = mem 'done'
;        addi    $v0, $zero, 0x0009        ; v0 = 0x0009  
;done:   jalr    $v0, $a2
;        halt                         ; stop
;
;array:  .word 20
;
;result: .word 0

        beq     $zero, $zero, sec
        addi    $zero, $at, 5
sec:    addi    $zero, $at, 10
        halt