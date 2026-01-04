				AREA Project, CODE, READONLY
				EXPORT __main

RS				EQU 0x20	; RS connects to PA5 (bit 5)
RW				EQU 0x40	; RW connects to PA6 (bit 6)
EN				EQU 0x80	; EN connects to PA7 (bit 7)
balance			EQU 0x64	; Initial balance set to $1000 (0x64)
usrTot			EQU 0x00	; Sets initial user card total to 0
houseTot		EQU 0x00	; Sets initial house card total to 0

genseed			DCD 0x12345678		;intial seed for random generation

deck			DCD  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18
				DCD  0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x21, 0x22, 0x23
				DCD  0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B
				DCD  0x2C, 0x2D, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36
				DCD  0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x41
				DCD  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49
				DCD  0x4A, 0x4B, 0x4C, 0x4D  ; 52 cards total
				
				;0x1X(Hearts) 0x2X(Diamonds) 0x3X(Clubs) 0x4X(Spades)
				;0xXA(10) 0xXB(Jack) 0xXC(Queen) 0xXD(King)

__main 			PROC
				BL LCDInit
				
				;Ports have been configured (remove this later)
				;User inputs: PA0, PA1.
				;LED Output: PA2-4
				;Reserved for LCD: PA5-7, PC0-7.
				
				;Load initial user and house card totals
				LDR R6, =usrTot			;loads initial user amount
				LDR R7, =houseTot		;loads initial dealer amount
				
				;Player specifies how much to bet
				BL pinCheck				;checks the state of the input buttons
				;CREATE FUNCTION TO SPECIFY PLAYER BET AMOUNT
				
				;Check user total; if less than 21, give option to keep hitting until stand or bust
				;CREATE FUNCTION TO ADD PLAYER CARDS AS ITS DRAWN
				CMP R6, 0x00000015		;compares user total with 21
				BNE pinCheck			;branches back to pincheck if not 21 yet

				
				;Check dealer total; if 16 or less, keep hitting. If 17 or more, stand. If bust, branch to win function.
				CMP R7, #0x00000010		;compares dealer total with 16
				BLE drawCard			;branches to drawCard if total is less than or equal to 16
				
				ENDP
				
				;Compare player and dealer totals. If player wins, branch to win function.
cardCheck		FUNCTION
				CMP R6, R7				;compares player and dealer totals (r6-r7)
				BGT Win					;branches to win function if player total is greater than dealer
				BLT Loss
				ENDP

				;Check state of input pins to see if user hit or stand (PA0 - hit) (PA1 - stand)
pinCheck		FUNCTION
				PUSH {R2, R3}
				LDRB R2, [R0, #0x10]	;loads GPIOA_IDR to R2
				AND R3, R2, #0x00000001 ;masks PA0 to isolate its value, stores value in R3
				CMP R3, #0x00000001 	;compares the masked value with 0x1 (check if PA0 is high)
				BEQ drawCard			;branches to drawcard if user hits
				AND R3, R2, #0x00000002	;masks Pa1 to isolate its value, stores value in R3
				CMP R3, #0x00000002		;compares the masked value with 0x2 (checks if PA1 is high)
				BEQ cardCheck
				BNE stay				;remains idle if user presses no button
				POP {R2, R3}
				BX LR

				ENDP

randNum			FUNCTION
				PUSH {R1, R2}
				LDR R1, =genseed		;loads the address of the seed
				LDR R8, [R1]			;R8 = current seed value
				
				;Linear Congruential Generator r8 = (r8 * a + c)
				MOV R2, #1664525		;multiplier (a)
				MUL R8, R8, R2			;multiplies r8 by a
				ADD R8, R8, #1013904223	;adds the constant c
				STR R8, [R1]			;updates the seed with the new value
				
				POP {R1, R2}
				BX LR
				ENDP
				;FINISHED, RANDOM NUMBER IS STORED IN R8/genseed

Win				FUNCTION
				PUSH {R4, LR}			;saves r4 and lr to the stack
				MOV R4, #0x03			;countdown for LED loop

celebrate		LDR R3, =0x00000004		;sends high signal to LED on PA2 and low to the rest
				STR R3, [R0, #0x14]		;writes to GPIOA_ODR
				BL delay				;calls delay function to give time for next led to come on
				
				LDR R3, =0x00000008		;sends high to LED on PA3
				STR R3, [R0, #0x14]
				BL delay
				
				LDR R3, =0x00000010		;sends high to LED on PA4
				STR R3, [R0, #0x14]
				BL delay
				
				SUBS R4, #1				;subtracts 1 from R4
				BNE celebrate			;Keeps going for 3 iterations
				POP {R4, LR}
				B stay					;remains idle until reset
				ENDP

Loss			FUNCTION
				MOV R2, #0x07			;clears the display and moves the cursor right 1 place
				PUSH {LR}
				BL LCDCommand
				
				MOV R3, #'Y'			; Character 'Y'	
				BL LCDData				; Send character in R3 to LCD
			
				MOV R3, #'O'			; Character 'O'	
				BL LCDData				; Send character in R3 to LCD
			
				MOV R3, #'U'			; Character 'U'	
				BL LCDData				; Send character in R3 to LCD

				MOV R2, #0xC7			;forces cursor to second line
				BL LCDCommand
				
				MOV R3, #'L'			; Character 'L'	
				BL LCDData				; Send character in R3 to LCD
			
				MOV R3, #'O'			; Character 'O'	
				BL LCDData				; Send character in R3 to LCD
			
				MOV R3, #'S'			; Character 'S'	
				BL LCDData				; Send character in R3 to LCD	

				MOV R3, #'E'
				BL LCDData
				
				POP {LR}
				
				ENDP
				
				;Initializes the GPIO clocks and registers for both LCD, user, and LED
LCDInit			FUNCTION
						
				; Enable GPIOA and GPIOC clocks
				LDR R0, =0x40023830      ; RCC_AHB1ENR address
				MOV R1, #0x00000005      ; Enable GPIOA (bit 0) and GPIOC (bit 2) clocks
				STR R1, [R0]             ; Write the value to RCC_AHB1ENR

				; Configure GPIOA and GPIOC
				LDR R0, =0x40020000		; GPIOA base address for control pins
				LDR R1, =0x40020800		; GPIOC base address for data pins 		
				LDR R2, =0x28005550     ; Set PA2, PA3, PA4, PA5, PA6, and PA7 as General Purpose Output Mode 0010 1000 0000 0000 0101 0101 0101 0000 keep PA13 & PA14 in alternative function mode
				STR R2, [R0, #0x00]		; Configure PA5, PA6, PA7 as output pins(01). PA0 and PA1 as input (00)
				
				LDR R2, =0x00015555			; Set PC0-PC7 as outputs (data pins)
				STR R2, [R1, #0x00]		; writes to GPIOC_MODER to set pins
				
				LDR R2, =0x0000000A		;Sets PA0 and PA1 as pull-down (active high) (1010)
				STR R2, [R0, #0x0C]		;writes to GPIOA_PUPDR to set pins

				PUSH {LR}		
				MOV R2, #0x38			; 2 lines, 7x5 characters, 8-bit mode		 
				BL LCDCommand			; Send command in R2 to LCD

				; ADD INSTRUCTIONS TO TURN ON THE DISPLAY AND THE CURSOR,
				MOV R2, #0x0E				;Turns on display and cursor
				BL LCDCommand
				
				; CLEAR DISPLAY AND MOVE CURSOR RIGHT
				MOV R2, #0x01				;Clears the display
				BL LCDCommand
				
				MOV R2, #0x06				;Moves cursor right 1 place
				BL LCDCommand
				
				POP {LR}			
				BX LR
				ENDP
					
LCDCommand	    FUNCTION				; R2 brings in the command byte
				STRB R2, [R1, #0x14]	; Send command to data pins (PC0-PC7)
				MOV R2, #0x00			; RS = 0, RW = 0, EN = 1
				ORR R2, EN
				STRB R2, [R0, #0x14]	; Set EN = 1 (enable pulse)
				PUSH {LR}
				BL delay
				
				MOV R2, #0x00
				STRB R2, [R0, #0x14]	; EN = 0, RS = 0, RW = 0
				POP {LR}
				BX LR
				ENDP				

				;COMPLETE THIS FUNCTION, REFER TO LCDCommand and TABLE 3 on HANDOUT
LCDData			FUNCTION				; R3 brings in the character byte
				STRB R3, [R1, #0x14]	;sends command to data pins
				MOV R3, #0x00			; RS = 1, RW = 0, EN = 1
				ORR R3, EN
				ORR R3, RS
				STRB R3, [R0, #0x14]	;Set EN = 1 (enable pulse)
				PUSH {LR}
				BL delay
				
				MOV R3, #0x00
				ORR R3, RS
				STRB R2, [R0, #0x14]	;EN = 0, RS = 1, RW = 0
				POP {LR}
				BX LR
				ENDP
					
stay			B stay
					
delay			FUNCTION
				MOV R5, #50
loop1			MOV R4, #0xFF
loop2			SUBS R4, #1
				BNE loop2
				SUBS R5, #1
				BNE loop1
				BX LR
				ENDP				

				
				
				END
				
	
