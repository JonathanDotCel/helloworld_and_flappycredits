
![](social_card_PNG.png)


# Hello world! + Flappy Credits
	
	Example bare-metal-ish code to get the pads + gpu going without using Sony's PS1 libraries.

	There's also a TTY device example to log printfs over Sio.

	First:
	Big thanks to Nicolas Noble over at https://github.com/grumpycoders/pcsx-redux/ for the toolchain!



# Installation

	Brief:

		Install docker
		Whitelist this folder
		Run the .bat

	Instructions:

		Install this: https://www.docker.com/

		Goto Settings | Resources | File Sharing, and add this folder

		Run buildme.bat to build.


# Questions

	Bare-metal-ish?
		The pads use the kernel

	Can I use some pre-made libraries?
		For PSYQ they must be converted with this tool:	https://github.com/grumpycoders/pcsx-redux/tree/main/tools/psyq-obj-parser
		For psn00b SDK, I'm not sure. Let me know how it goes!



