# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# to include the font data and various other parts
# without converting them to header files
	
	.section .text.wrapper, "x", @progbits
	
	.set push
	.set noreorder
    
	.global xfont
	.global lobster_tim
	.global octo_angery_tim
	.global octo_happy_tim
	
	
	.align 4			
	
lobster_tim:
		.incbin "bins/lobster.tim"
		.align 4

        # so angery
octo_angery_tim:
		.incbin "bins/octo_angery.tim"
		.align 4
	
octo_happy_tim:
		.incbin "bins/octo_happy.tim"
		.align 4

xfont:
		.incbin "bins/inc_font.tim"
		.align 4
	
	.set pop


