
#
#
# TTY Redirect
# Reserves 0xC000 to 0xC200
# (Currently about 0xC100 - 0x120# 75 instructions)
#
# OLD way: Redirects printfs to SIO via B(3Dh) std_out_putchar(char)
# New way: tty device
#
#

	.set push
	.set noreorder

	.global installtty
	.global isttyinstalled


	.equ	SIO_DATA,	0x1050
	.equ	SIO_STAT,	0x1054
	.equ	SIO_MODE,	0x1058
	.equ	SIO_CTRL,	0x105A
	.equ	SIO_BAUD,	0x105E
	.equ	SI_STAT,	0x1070
	.equ	SI_MASK,	0x1074

# File control block equates
# E.g. offset from the addr
	.equ	FCB_STATUS,	0x00	# 0 = free, !0 = accessmode
	.equ	FCB_DISKID,	0x04	# cd table or card port
	.equ	FCB_TADDR,	0x08	# transfer addr
	.equ	FCB_TLEN,	0x0C	# transfer length
	.equ	FCB_FPOS,	0x10	# file pos
	.equ	FCB_DFLAGS,	0x14	# device flags
	.equ	FCB_ERROR,	0x18	# last error B(55)
	.equ	FCB_DCB,	0x1C # pointer to file's DCB
	.equ	FCB_FSIZE,	0x20	# file size
	.equ	FCB_LBN,	0x24 # lba (cdrom)
	.equ	FCB_FCBID,	0x28 # ID 0-15 (same as handle?)


# a0 = addr of ttyDevice struct
sys_installdevice:	
	li	$t2, 0xB0	
	jr	$t2
	li	$t1, 0x47

# a0 = lowercase name
sys_removedevice:	
	li	$t2, 0xB0	
	jr	$t2
	li	$t1, 0x48
	nop

sys_printdevices:	
	li	$t2, 0xB0	
	jr	$t2
	li	$t1, 0x49
	nop

# a0 = handle
sys_closehandle:
	li	$t2, 0xB0	
	jr	$t2	
	li	$t1, 0x36
	nop

# a0 = filename
# a1 = access mode
sys_openfile:
	li	$t2, 0xB0	
	jr	$t2
	li	$t1, 0x32
	nop


# B0,57 - GetB0table
getB0Table:	
	li	$t2, 0xB0	
	jr	$t2
	li	$t1, 0x57
	nop
	

isInstalled:
		.long	0


isttyinstalled:

		la		$v0, isInstalled
		lw		$v0, 0($v0)
		jr		$ra
		nop


# switched to device-based as of B6

installtty:
		
		addiu	$sp, $sp, -0x10
		sw	$ra, 0x00($sp)
		sw	$a0, 0x04($sp)
		sw	$a1, 0x08($sp)
		sw	$a1, 0x0C($sp)
		
	
	# now copy our code cave into C000
		li	$a0, 0xC000
		la	$a1, ttyCopyStart
		la	$a2, ttyCopyEnd
		subu	$a2, $a2, $a1		# size = end - start	
		jal		SYS_memcpy
		nop

	# Install the TTY device

		# these guys need a lil' extra stack		
		addiu $sp, $sp, -0x8

		# close stdin
		li	$a0, 0
		jal sys_closehandle
		nop
		
		# close stdout
		li	$a0, 1
		jal	sys_closehandle
		nop
		
		# remove the dummy tty device
		la	$a0, deviceName
		jal	sys_removedevice
		nop

		# install the new one
		la	$a0, ttyDevice
		jal	sys_installdevice
		nop

		# point stdin at the new device
		la $a0, deviceName
		li $a1, 2
		jal sys_openfile
		nop

		# point stdout at the new device
		la $a0, deviceName
		li $a1, 1
		jal sys_openfile
		nop

		addiu $sp, $sp, 0x8

	# Optional
		jal sys_printdevices
		nop
	
		
	# Debug# how big is this section of code?
		la	$a1, ttyCopyStart
		la	$a2, ttyCopyEnd
		subu	$v0, $a2, $a1		# size = end - start	

		# store any nonzero thing into that var
		la	$a0, isInstalled;
		sw	$a0, 0($a0)
		nop

_it_alreadyInstalled:

		lw	$ra, 0x00($sp)
		lw	$a0, 0x04($sp)
		lw	$a1, 0x08($sp)
		lw	$a2, 0x0C($sp)
		jr	$ra
		addiu	$sp, $sp, 0x10



deviceName:	
			.string	"tty"
			.align	0

deviceDesc:
			.string	"SIO TTY"
			.align	4

# These aren't working properly in the ttyDevice struct
	.equ	DV_SENDCHAR,	0xC000
	.equ	DV_SENDSTR,		0xC008
	.equ	CHARCAVE,		0xC010		#old hook addr, can repurpose
	.equ	DV_INOUT,		0xC018
	.equ	DV_DUMMY,		0xC020


ttyDevice:
	.long	deviceName	# 
	.long	3			# flags
	.long	0			# blockSize
	.long	deviceDesc	# 
	.long	0xC020	# init
	.long	0xC028	# open
	.long	0xC018	# action
	.long	0xC028	# close
	.long	0xC028	# ioctl
	.long	0xC020	# read
	.long	0xC020	# write
	.long	0xC020	# erase
	.long	0xC020	# undelete
	.long	0xC020	# firstfile
	.long	0xC020	# nextfile
	.long	0xC020	# format
	.long	0xC020	# chdir
	.long	0xC020	# rename
	.long	0xC020	# deinit/remove
	.long	0xC020	# check?




ttyCopyStart:
		

		# 0xC000
		b sendCharTTY
		nop

		# 0xC008
		b sendStringTTY
		nop
		
		# 0xC010
		#b putCharCave
		nop
		nop
		
		# 0xC018 - ioctl
		b actionTTY
		nop
		
		# 0xC020 - dummy
		jr	$ra
		nop
		
		# 0xC028 - returns 0
		jr	$ra
		addu	$v0, $0, $0
		

actionTTY:
	
	addiu	$sp, $sp, -0x8
	sw	$ra, 0x0($sp)
	sw	$t0, 0x4($sp)
	nop
	
	# Read = 1
	# Write = 2
	li	$t0, 2
	bne	$a1, $t0, _at_notWriting
	nop

	bal sendStringTTY
	nop

_at_notWriting:

	li	$v0,0
	nop

	lw		$ra, 0x0($sp)
	lw		$t0, 0x4($sp)
	nop
	jr		$ra
	addiu	$sp, $sp, 0x8


sendStringTTY:
	
	addiu	$sp, $sp, -0x10
	sw	$ra, 0x0($sp)
	sw	$t0, 0x4($sp)
	sw	$t1, 0x8($sp)
	sw	$a0, 0xC($sp)
	nop
	
	
	lw	$t1, FCB_TLEN($a0)	# transfer length
	lw	$t0, FCB_TADDR($a0)	# addr to read
	addu $t1, $t1, $t2			# t1 = stop reading here
	

_ss_readAnother:
	lbu	$a0, 0($t0)
	nop

	bal sendCharTTY
	nop
	addiu $t0, $t0, 1

	bne	$t0, $t1, _ss_readAnother
	nop
	
	lw	$ra, 0x0($sp)
	lw	$t0, 0x4($sp)
	lw	$t1, 0x8($sp)
	lw	$a0, 0xC($sp)
	jr	$ra
	addiu	$sp, $sp, 0x10


#
# SendChar
#
sendCharTTY:


	addiu	$sp, $sp, -0x10
	sw	$ra, 0x0($sp)
	sw	$t0, 0x4($sp)
	sw	$t1, 0x8($sp)
	sw	$t2, 0xC($sp)
	nop

	# t0 = timer
	# t1 = hw base
	# t2 = temp

	# fixed delay
	li $t0, 0x20
_sc_wasteMore:
	addiu	$t0, $t0, -1
	bnez	$t0, _sc_wasteMore
	nop
	
	lui	$t1, 0x1F80
	li	$t0, 0x20		# max wait 2000 incase SIO gums up
	

_sc_keepWaiting:
	
	lbu		$t2, SIO_STAT($t1)
	nop
	andi	$t2, $t2, 0x5			# SR_TXU | SR_TXRDY#
	nop

	#beqz	$t0, @bailout
	#subiu	$t0, $t0, 1

	beqz	$t2, _sc_keepWaiting
	nop
_sc_bailout:
	
	sb	$a0, SIO_DATA($t1)

	lw	$ra, 0x0($sp)
	lw	$t0, 0x4($sp)
	lw	$t1, 0x8($sp)
	lw	$t2, 0xC($sp)
	jr	$ra
	addiu	$sp, $sp, 0x10

	



ttyCopyEnd:

SYS_memcpy:
		addiu   $t2,$0,0xA0
        jr      $t2
        addiu   $t1,$0,0x2A


	.set pop


