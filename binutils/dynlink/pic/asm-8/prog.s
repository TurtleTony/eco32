	.export	_start

	.export	dat1
	.export	dat2
	.export	dat3
	.export	dat4
	.export	dat5

	.code

_start:	.gotadr	$3
	.gotptr	$1,$3,dat3
	ldw	$8,$1,0
	.gotptr	$1,$3,dat4
	ldw	$7,$1,0
	.gotptr	$1,$3,dat3
	ldw	$6,$1,0
	.gotptr	$1,$3,dat4
	ldw	$5,$1,0
loop:	j	loop

	.data

dat1:	.word	0x12345678
dat2:	.word	0x23456789
dat3:	.word	0x3456789A
dat4:	.word	0x456789AB
dat5:	.word	0x56789ABC
