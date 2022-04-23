; Print.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly
; Runs on TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; ST7735_OutChar   outputs a single 8-bit ASCII character
; ST7735_OutString outputs a null-terminated string 

    IMPORT   ST7735_OutChar
    IMPORT   ST7735_OutString
		
	PRESERVE8
		
    AREA    |.text|, CODE, READONLY, ALIGN=2
    THUMB



;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
NUMA EQU 0

LCD_OutDec PROC
    EXPORT   LCD_OutDec
;  1) allocate local variable n on the stack
    PUSH {R0-R11,LR}
; put your lab 7 here
	CMP R0, #10
	BLO base
	MOV  R1,#10
    UDIV R2,R0,R1
    MUL  R2,R2,R1
    SUB  R3,R0,R2
	ADD R3, #0x30
	MOV R1, R3
	
	PUSH {R1, LR}	;STR R1, [SP, #num]
	SUB SP, #4	
	STR R1, [SP, #NUMA]
	
	MOV R1, #10
	UDIV R0, R0, R1
	BL LCD_OutDec

	LDR R0, [SP, #NUMA]	;issues with locals
	ADD SP, #4	
	POP{R0, LR}
	PUSH{LR}
    BL ST7735_OutChar
	POP{PC}
    BX  LR
	
base
	ADD R0, #0x30
	PUSH{LR}
    BL ST7735_OutChar
    POP  {R0-R11,PC}
	BX  LR

    ENDP
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.01, range 0.00 to 9.99
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.00 "
;       R0=3,    then output "0.03 "
;       R0=89,   then output "0.89 "
;       R0=123,  then output "1.23 "
;       R0=999,  then output "9.99 "
;       R0>999,  then output "*.** "
; Invariables: This function must not permanently modify registers R4 to R11
FP RN 11
COUNT EQU 0
NUM EQU 4

LCD_OutFix PROC
         EXPORT   LCD_OutFix
;  0)save any registers that will be destroyed by pushing on the stack
;  1)allocate local variables letter and num on the stack

; put your lab 7 here
	PUSH{R4-R9, R11, LR}
    SUB SP, #8

    LDR R4, =9999
    CMP R0, R4
    BHI TOOBIG

     MOV FP, SP
    MOV R4, #0
    STR R4, [FP, #COUNT]
    STR R0, [FP, #NUM]
    MOV R5, #10
LOOP
    LDR R4, [FP, #NUM]
    CMP R4, #0
    BEQ Next1
    UDIV R6, R4, R5
    MUL R7, R6,    R5
    SUB R8, R4, R7
    PUSH{R8}

    STR R6, [FP, #NUM]
    LDR R6, [FP, #COUNT]
    ADD R6, #1
    STR R6, [FP, #COUNT]
    B LOOP

Next1
    LDR R6, [FP, #COUNT]
    CMP R6, #4
    BEQ Next2
    MOV R7, #0
    PUSH{R7}
    ADD R6, #1
    STR R6, [FP, #COUNT]
    B Next1

Next2
    LDR R4, [FP, #COUNT]
    CMP R4, #3
    BNE SKIP
    MOV R0, #0x2E
    BL ST7735_OutChar
SKIP
    CMP R4, #0
    BEQ Done
    POP {R0}
    ADD R0, #0x30

    BL ST7735_OutChar

    SUB R4, #1
    STR R4, [FP, #COUNT]
    B Next2

TOOBIG
    MOV R0, #0x2A
    BL ST7735_OutChar
    MOV R0, #0x2E
    BL ST7735_OutChar
    MOV R0, #0x2A
    BL ST7735_OutChar
    MOV R0, #0x2A
    BL ST7735_OutChar
    MOV R0, #0x2A
    BL ST7735_OutChar

Done
    ADD SP, #8
    POP{R4-R9, R11, PC}
    BX   LR
         ENDP

;* * * * * * * * End of LCD_OutFix * * * * * * * *

    ALIGN                           ; make sure the end of this section is aligned
    END                             ; end of file
