	.export	loop
	.import	dat1
	.export	dat2
	.import	dat3
	.export	dat4
	.import dat5

	.code
	.ldgot	$3
    ldw	$8,$0,dat2
    ldw	$9,$0,dat4
    ldw	$10,$0,dat2
    ldw	$11,$0,dat4
    ;ldw	$12,$0,dat1
    ;ldw	$13,$0,dat3
loop:	j	loop

    .data
dat2:   .word 0x23456789
dat4:   .word 0x456789AB