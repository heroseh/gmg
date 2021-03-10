#!/bin/sh

if test ! -f "$1.c"; then
	echo "there are no examples with the file name \"$1\""
	echo "here are a list of the examples:"
	ls | grep ".c" | cut -f 1 -d '.'
	exit 1
fi

if test -f "bin/$1"; then
	rm bin/$1
fi

mkdir -p bin

glslc $1.vert -o bin/$1.vert.spv
glslc $1.frag -o bin/$1.frag.spv
clang -o bin/$1 $1.c -g -lm -lvulkan -lSDL2 -fsanitize=address

if test -f "bin/$1"; then
	./bin/$1
fi


