;
; start.s -- startup code
;

	.export	_start
	.import	main

	.code

_start:
	add	$29,$0,stack	; set sp
	jal	main		; call 'main'
start1:
	j	start1		; loop

	.bss

	.align	4
	.space	0x800
stack:
