// could have avoided repetition

(LOOP)
	@SCREEN
	D=A
	@addrPointer 
	M=D

	@KBD
	D=M 
	@WHITE
	D;JEQ
	@BLACK
	0;JMP


(BLACK)
	@addrPointer 
	D=M 
	@24576

	D=D-A
	@LOOP
	D;JEQ

	@addrPointer 
	D=M
	M=M+1

	@addr
	A=D
	M=-1
	@BLACK
	0;JMP


(WHITE)
	@addrPointer 
	D=M 
	@24576

	D=D-A
	@LOOP
	D;JEQ

	@addrPointer 
	D=M
	M=M+1

	@addr
	A=D
	M=0
	@WHITE
	0;JMP

