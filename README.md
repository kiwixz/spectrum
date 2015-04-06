# spectrum

## Description

This is a simple audio spectrum visualizer written in C. It's only tested on Linux (more precisely Arch Linux).

## What you need

An usual development environnement (a C compiler, GNU Make, etc) is needed. The Makefile use `c99` as an alias for the compiler. You need youtube-dl to open URLs.

This program also need some libraries:
- GLEW
- GTK+ 2
- GtkGLExt
- GStreamer
- libtiff

Finally, you need an **m4a** audio file or an URL to a video from a youtube-like site.

## Compile and Run

There is a Makefile so you would compile and run it with only one command:

```
make run
```

## Features

- Press `O` to open a file or download the audio from a video from youtube-like sites.