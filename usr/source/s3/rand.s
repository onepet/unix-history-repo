/ random fixed point number generator

.globl	rand, srand

srand:
	mov	r0,ranx
	rts	pc

rand:
	mov	r1,-(sp)
	mov	ranx,r1
	mpy	$13077.,r1
	add	$6925.,r1
	mov	r1,r0
	mov	r0,ranx
	bic	$100000,r0
	mov	(sp)+,r1
	rts	pc

.data
ranx:	1
