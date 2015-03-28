# spectrum

## Description

This is a simple audio spectrum visualizer written in C. It's only tested on Linux (more precisely Arch Linux).

## What you need

An usual development environnement (a C compiler, GNU Make, etc) is needed. The Makefile use `c99` as an alias for the compiler.

This program also need some libraries:
- GLEW
- GTK+ 2
- GtkGLExt
- GStreamer

Finally, you need an **m4a** audio file to run it.

## Compile and Run

There is a Makefile so you would compile and run it with only one command, passing the audio file as argument like that:

```
make run ARGS="foo.m4a"
```
