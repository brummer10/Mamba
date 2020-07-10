# MidiKeyBoard
Virtual Midi keyboard for Jack Audio Connection Kit

![MidiKeyBoard](https://github.com/brummer10/MidiKeyBoard/raw/master/MidiKeyBoard.png)


## Features

- Virtual Midi Keyboard for Jack Audio Connection Kit
- Midi Through: forward midi input to output
- [NSM](https://linuxaudio.github.io/new-session-manager/) support

## Dependencys

- libcairo2-dev
- libx11-dev
- liblo-dev
- libjack-(jackd2)-dev

## Build

- git submodule init
- git submodule update
- make
- sudo make install # will install into /usr/bin
