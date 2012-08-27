# WPGenerator
WPGenerator is a simple wallpaper generator for Arch Linux.
You can use it to create differently sized wallpapers.

# Dependencies
* cairo
* librsvg

# Setup
	git clone git://github.com/thillux/WPGenerator.git
	cd WPGenerator
	make
# Usage
If you want to create a 1920x1200 wallpaper, type:

	./WPGenerator --width 1920 --height 1200

The number of circles and waves are optional. The can be chosen explicitly:

	./WPGenerator --width 1920 --height 1200 --circles 50 --waves 3

or implicitly:
	
	./WPGenerator --width 1920 --height 1200 --random

# TODO
* logo resizing
* faster wave drawing

