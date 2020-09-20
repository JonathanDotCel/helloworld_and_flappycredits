# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# to include the gpu font and for various asm tests
	
	.section .text.wrapper, "x", @progbits
	

	.set push
	.set noreorder
	
	# inc_font.tim
	.global xfont
			
	
# Don't need to crunch it, as the final .exe is crunched
# typical 7kb -> 1kb
xfont:
		.incbin "bins/inc_font.tim"
		.align 4
	
	.set pop


