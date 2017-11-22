#!/bin/sh
files="screenrc termcolors vimrc Xmodmap Xresources zshrc"
for f in $files ; do
	echo $f
	ln -vs $(pwd)/$f ~/.$f
done

