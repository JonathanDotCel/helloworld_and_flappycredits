




# UniROM without Caetla

			#.section .start, "ax", @progbits
			.section .text

			.global __start
			
			.set noreorder
			
__start:

			# Diamond Screen -> Black screen -> HERE <- game start
			# e.g. second boot hook
			.org		0x00000000											#header, points to EP2
			.long		entryPoint2
			.string		"DISABLED by Sony Computer Entertainment Inc."		# main .exe checks this string to determine feature set			
			.string		"Unirom is not licensed by Sony."
					
			# Code wedged into the header for debugging.
			.org		0x00000060
			la			$t0, entryPoint1
			jr			$t0
			nop

			# The initial boot right as the sytem turns on.
			# e.g. first boot hook
			.org		0x00000080		#0x80 points to EP1
			.long		entryPoint1
			.string		"Licensed by Sony Computer Entertainment Inc."
			.string		"Unirom is not licensed by Sony"
			
			# So we can always jump to  0x1F020100 to start
			# Caetla's header will rely on this being here.			
			# removing this will prevent booting as the
			# *actual* header above will be ignored
			.org		0x00000100	
			la			$t0, entryPoint2
			jr			$t0
			nop

			# Same for the other entry point
			# (incase you intend to use 2nd ep)
			.org		0x00000120	
			la			$t0, entryPoint1
			jr			$t0
			nop
			

			# 256 bytes reserved for settings, patches, future expansion
			.org		0x00000200

reserved:
			.long		0

			.org		0x00000300

# Does this build have caetla first on the cart?
# could use an equ, but ASMPSX gets buggy and caches vars.
hascaetla:
			.long	0		

# helps find start locations when debugging and manually patching
marker:
			.string		"MARKER00"
			.align 4	

l_shared:
			.include	"rom_shared.s"
			.align 4
			
Ending:	


			nop
			nop
