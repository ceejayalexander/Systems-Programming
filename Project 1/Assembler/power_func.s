;; Write a function in LC-2200 assembly language 
;; to compute pow(n, m), that is, n raised to the mth
;; power. (For Example, pow(2, 4) should return 16.

;; Reference:
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

;;
; pow(n, m):
;       if m > 0:
; 		return n * pow(n, m - 1);
; 	return 1;
	
; pow(3, 4)
; 	return 3 * pow(3, 3)
; 		pow(3, 3)
; 			return 3 * pow(3, 2)
; 				pow(3, 2)
; 					return 3 * pow(3, 1)
; 						pow(3,1)
; 							returns 3
; 					return 3 * 3 = 9
; 			return 3 * 9 = 27
; 	return 3 * 27 = 81

lw $at, start($zero) ; $at = 0x3050
lw $sp, stack_ptr($zero) ; sp = 0x7000
lw $t0, pow($zero) ; t0 = 0x305F
jalr $at, $ra ; Program starts at addr = 12368 (0x3050)

start:     .dw 12368
stack_ptr: .dw 28672
pow:       .dw 12383

; 1. Caller saves t0-t2 temporary registers on the stack
addi $sp, $sp, -3 ; Allocate space for t0, t1, t2 | addr = 0x3050
sw $t0, 2($sp)    ; Save t0
sw $t1, 1($sp)    ; Save t1
sw $t2, 0($sp)    ; Save t2

; 2. Caller places parameters in a0-a2
lw $a0, 8($at) ; a0 = base (n)
lw $a1, 9($at)  ; a1 = exp (m)

; 3. Caller allocates space for "additional" return values on stack
; Return value will be stored in $v0

; 4. Caller saves previous return address currently in ra
; Doesn't apply for 'main()' function

; 5. Caller executes JALR at t0 = addr 'pow'
add $a2, $zero, $t0
jalr $t0, $ra ; Branch to 'pow' function

base:   .word 2
exp:    .word 4

; 11. Caller restores previous return address to ra
; Doesn't apply since previous return address never pushed onto stack

; 12. Caller stores additional return values as desired
; Doesn't apply return value for 'pow' is stored in v0

; 13. Caller moves stack pointer to discard additional parameters
; Doesn't apply since additional parameters were not pushed

; 14. Caller restores any saved t0-t2 registers from stack
lw $t2, 0($sp)  ; Restore t2
lw $t1, 1($sp)  ; Restore t1
lw $t0, 2($sp)  ; Restore t0
addi $sp, $sp, 3 ; Pop t0-t2 off stack

halt

; int pow(int n, int m) {
;     if (m > 0) {
;         return n * pow(n, m - 1);
;     }
;     return 1;
; }

; int pow(int n, int m) {
;     if (m == 0) {
;         return 1;
;     }
;     return n * pow(n, m - 1);
; }

pow: 
        ; 6. Callee stores previous frame pointer on stack and current frame pointer in register
        addi $sp, $sp, -1
        sw $pr, 0($sp)  ; Save old frame pointer
        add $pr, $sp, $zero ; Set current frame pointer

        ; 7. Callee saves any of registers s0-s2 that it plans to use during its execution on the stack
        addi $sp, $sp, -3
        sw $s0, 2($sp)
        sw $s1, 1($sp)
        sw $s2, 0($sp)

        ; 8. Callee allocates space for any local variables on stack
        ; DOES NOT APPLY

        beq $a1, $zero, base_case ; if (m == 0) return 1;
        beq $zero, $zero, pow_recurse ; else execute recursion

base_case:
        addi $v0, $zero, 1 ; rv = 1
        
        ; 9. Prior to return, callee restores any saved s0-s2 registers from stack
        lw $s2, 0($sp)
        lw $s1, 1($sp)
        lw $s0, 2($sp)
        addi $sp, $sp, 3

        lw $pr, 0($sp) ; Restore old fp
        addi $sp, $sp, 1

        ; 10. Callee executes jump to ra
        jalr $ra, $zero ; Recall that zero register always outputs 0, no matter what's written, so this
                        ; safely gets rid of the unneeded return addr in $zero

pow_recurse:
        addi $sp, $sp, -3 ; Allocate space for t0, t1, t2
        sw $t0, 2($sp)    ; Save t0
        sw $t1, 1($sp)    ; Save t1
        sw $t2, 0($sp)    ; Save t2

        ; Push n and m-1 in reverse order
        addi $sp, $sp, -2 ; Allocate space for m-1 on stack
        addi $a1, $a1, -1 ; a1 = m-1
        sw $a0, 0($sp) ; Push a0 = n onto stack
        sw $a1, 1($sp) ; Push a1 = m-1 onto stack
        
        addi $sp, $sp, -1 ; Allocate space for ra on stack
        sw $ra, 0($sp) ; Push prev ra on stack

        jalr $a2, $ra  ; Recurse and go back to pow

        lw $ra, 0($sp) ; Restore old ra
        lw $a0, 1($sp) ; a0 = n
        lw $a1, 2($sp) ; a1 = m-1
        addi $sp, $sp, 3 ; Pop m-1 and n off stack

        lw $s2, 0($sp)
        lw $s1, 1($sp)
        lw $s0, 2($sp)
        addi $sp, $sp, 3

        ; n * pow(n, m-1) logic here:
        addi $t0, $zero, 0 ; t0 = 0 (result accumulator)
        add $t1, $zero, $a0 ; t1 = n
        add $t2, $zero, $v0 ; t2 = pow(n, m-1)

mult_loop:
        beq $t1, $zero, done_mult ; if t1 = 0, branch 'done_mult'
        add $t0, $t0, $t2 ; add 'pow(n, m-1)' n times
        addi $t1, $t1, -1 ; Decrement n
        beq $zero, $zero, mult_loop ; branch unconditionally back to mult_loop

done_mult:
        add $v0, $zero, $t0 ; Store final result

        ; Prior to return, callee restores any saved s0-s2 registers from stack
        lw $s2, 0($sp)
        lw $s1, 1($sp)
        lw $s0, 2($sp)
        addi $sp, $sp, 3

        lw $pr, 0($sp)   ; Restore old fp
        addi $sp, $sp, 1 ; Pop fp off stack

        ; Callee executes jump to ra
        jalr $ra, $zero




