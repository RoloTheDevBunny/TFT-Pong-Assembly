.eqv FB, 0x10010000
.eqv W,  256
.eqv H,  256
.eqv CircleR 20
.eqv BLACK,   0x00000000
.eqv WHITE,   0x00FFFFFF
.eqv RED,     0x00FF0000
.eqv GREEN,   0x0000FF00
.eqv BLUE,    0x000000FF
.eqv YELLOW,  0x00FFFF00
.eqv CYAN,    0x0000FFFF
.eqv MAGENTA, 0x00FF00FF

.text
.globl main

main:
	li s0, 128
	li s1, 128

	j RenderPipeline

FinishRender:
	li a7, 10
	ecall

# RARS RENDER START
# a0 = framebuffer address
# a1 = width
# a2 = height

# s0 = circle x position
# s1 = circle y position

# t0 = current address
# t1 = x position
# t2 = y position
# t3 = color

RenderPipeline:
RenderBackground:
	li t0, FB
	li a1, W
	li a2, H
	li t1, 0

RBLoopY:
	bge t2, a2, RBEndLoopY

	li t1, 0
	slli t3, t2, 8

RBLoopX:
	bge t1, a1, RBEndLoopX
	

	sw t3, 0(t0)

	addi t0, t0, 4
	addi t1, t1, 1
	addi t3, t3, 1
	j RBLoopX

RBEndLoopX:
	addi t2, t2, 1
	j RBLoopY

RBEndLoopY:
	j RenderCircle

# t0 = x position
# t1 = y position
# t2 = finish position
# t3 = color
# t4 = write address

# a0 = dx / dx + dy
# a1 = dy
# a2 = CircleR
# a3 = CircleR^2
RenderCircle:
	li a2, CircleR
	mul a3, a2, a2
	sub t0, s0, a2
	sub t1, s1, a2
	
RCLoopY:
	addi t1, t1, 1
	sub a1, t1, s1
	mul a1, a1, a1
	
	add t2, s1, a2
	bge t1, t2, FinishRender
	
	add t2, s0, a2
	sub t0, s0, a2
RCLoopX:
	addi t0, t0, 1
	bge t0, t2, RCLoopY

	sub a0, t0, s0
	mul a0, a0, a0
	
	add a0, a0, a1
	bgtu a0, a3, RCLoopX
RCColorIn:
	# color white
	li t3, W
	mul t4, t1, t3
	add t4, t4, t0
	slli t4, t4, 2
	
	li t3, FB
	add t4, t4, t3
	
	li t3, WHITE
	sw t3, 0(t4)
	j RCLoopX
	
	
	
	
	
	
	
# UNUSED HELPERS

# a0 = x position
# a1 = y position
# a2 = radius
IsInRadius:
	sub a0, a0, a2 # a0 = a0 - r
	mul a0, a0, a0 # a0 = a0 * a0
	sub a1, a1, a2 # a1 = a1 - r
	mul a1, a1, a1 # a1 = a1 * a1
	
	add t0, a0, a1 # t0 = a0 + a1
	mul a2, a2, a2 # a2 = a2 * a2
	bltu t1, t0, IIROutside # if t1 < t0 then go to outside

IIRInside:
    li   a0, 1
    ret

IIROutside:
    li   a0, 0
    ret