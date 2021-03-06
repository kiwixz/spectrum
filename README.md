# spectrum

[Feel free to contact me by email.](mailto:kiwixz@users.noreply.github.com)

## Description

This is a simple audio spectrum visualizer written in C using GStreamer. It's only tested on Linux (more precisely Arch Linux).

## What you need

You need an usual development environnement (a C compiler, GNU Make, etc). The Makefile use `c99` as an alias for the compiler. You need youtube-dl to download audio from URLs and FFmpeg for recording a video with sound.

This program also need some libraries:
- GLEW
- GTK+ 2
- GtkGLExt
- GStreamer
- libtiff
- OpenCV

Finally, you need an audio file readable by GStreamer, or an URL to a video from a youtube-like site.

## Compile and Run

There is a Makefile so you would compile and run it with only one command:

```
make run
```

## Shortcuts

- SPACE - play / pause
- ENTER - record / stop recording

## License
This program is protected by the **GNU GENERAL PUBLIC LICENSE**. This repositery should contain a file named _LICENSE_.
