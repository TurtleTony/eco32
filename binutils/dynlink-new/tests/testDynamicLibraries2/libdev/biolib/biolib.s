	.code
	.export	getc
	.align	4
getc:
	sub	$29,$29,16
	stw	$23,$29,0
	add	$23,$0,0xf0300000
L.2:
L.3:
	ldw	$24,$23,0
	and	$24,$24,1
	beq	$24,$0,L.2
	ldw	$24,$23,4
	stb	$24,$29,-1+16
	ldb	$2,$29,-1+16
L.1:
	ldw	$23,$29,0
	add	$29,$29,16
	jr	$31

	.export	putc
	.align	4
putc:
	sub	$29,$29,16
	stw	$23,$29,0
	add	$23,$0,0xf0300000
L.6:
L.7:
	ldw	$24,$23,8
	and	$24,$24,1
	beq	$24,$0,L.6
	sll	$24,$4,8*(4-1)
	sar	$24,$24,8*(4-1)
	stw	$24,$23,12
L.5:
	ldw	$23,$29,0
	add	$29,$29,16
	jr	$31

