# Mamba
Virtual Midi keyboard for Jack Audio Connection Kit

![Mamba](https://github.com/brummer10/Mamba/raw/master/Mamba.png)


## Features

- Virtual Midi Keyboard for [Jack Audio Connection Kit](https://jackaudio.org/)
- Midi Through: forward midi input to output and display incomming note events on the keyboard
- [NSM](https://linuxaudio.github.io/new-session-manager/) support

## Dependencys

- libcairo2-dev
- libx11-dev
- liblo-dev
- libsig++-2.0-dev
- libjack-(jackd2)-dev

## Build

- git submodule init
- git submodule update
- make
- sudo make install # will install into /usr/bin
